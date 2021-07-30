#include "MCU_IO.h"
#include "main.h"
#include "gsm.h"

#include "stm324xg_eval_sdio_sd.h"

typedef enum
{
  FAILED = 0,
  PASSED = !FAILED
} TestStatus;
extern volatile bool data_ready;
extern SD_CardInfo SDCardInfo;
/* Private define ------------------------------------------------------------*/

#define BLOCK_SIZE 512
#define MULTI_BUFFER_SIZE 100
#define NUMBER_OF_BLOCKS 100

#define SD_OPERATION_ERASE 0
#define SD_OPERATION_BLOCK 1
#define SD_OPERATION_MULTI_BLOCK 2
#define SD_OPERATION_END 3

// Global IIR variables
float32_t biquad_HP_State[ADS1299_CHANNELS][2 * BIQUAD_STAGES_HP];
arm_biquad_cascade_df2T_instance_f32 biquad_HP_Struct[ADS1299_CHANNELS];

float32_t biquad_BP_State[ADS1299_CHANNELS][2 * BIQUAD_STAGES_BP];
arm_biquad_cascade_df2T_instance_f32 biquad_BP_Struct[ADS1299_CHANNELS];
/* Private macro -------------------------------------------------------------*/
/* Private variables ---------------------------------------------------------*/
uint8_t aBuffer_Block_Tx[BLOCK_SIZE];
uint8_t aBuffer_Block_Rx[BLOCK_SIZE];
//uint8_t aBuffer_MultiBlock_Tx[MULTI_BUFFER_SIZE];
uint8_t aBuffer_MultiBlock_Rx[MULTI_BUFFER_SIZE];

__IO uint32_t uwLsiFreq = 0;

__IO TestStatus EraseStatus = FAILED;
__IO TestStatus TransferStatus1 = FAILED;
__IO TestStatus TransferStatus2 = FAILED;

SD_Error Status = SD_OK;
__IO uint32_t uwSDCardOperation = SD_OPERATION_ERASE;

/* Private function prototypes -----------------------------------------------*/
static void NVIC_Configuration(void);
static void SD_EraseTest(void);
static void SD_SingleBlockTest(void);
static void SD_MultiBlockTest(void);
static void Fill_Buffer(uint8_t *pBuffer, uint32_t BufferLength, uint32_t Offset);

static TestStatus Buffercmp(uint8_t *pBuffer1, uint8_t *pBuffer2, uint32_t BufferLength);
static TestStatus eBuffercmp(uint8_t *pBuffer, uint32_t BufferLength);

volatile uint8_t Timeout = 0; //The TIM3 IRQ changes this flag to 1, so that the program does'nt get stuck in a loop.
uint8_t Active_Connection;    //Specifies the Active connection for communication.ie.WIFI,GSM or Satellite.
uint8_t Count = 0;            //The value of Retries done.
uint8_t ld = 0;
extern uint8_t Get_Parameters; //Flag for getting GNSS Data.
extern uint8_t gsm_ready;      //Flag to check if the UART of the GSM module is ready for communication.
extern uint8_t gsmNtwk;        //The Status of GSM Network.ie. Connected,Searching etc.
extern struct _server          //Structure for holding server Parameters.
{
  uint8_t IP;       //IP of the Server.
  uint8_t PORT;     //Port of the Server.
  uint8_t USER[50]; //MQTT username.
  uint8_t PSWD[50]; //MQTT Password.
  uint8_t CID[50];  //The Client ID tobe sent to MQTT Server.
} server;

extern enum cmd_type { cmd_null,
                       cmd_APN,
                       cmd_PDP
} cmd_type;

extern uint8_t MQTT_ConnectPacket[200]; //The Connect Packet for MQTT.

extern uint8_t MQTT_PublishPacket[200]; //The Publish Packet for MQTT.

extern uint8_t MQTT_SubscribePacket[200]; //The Subscribe Packet for MQTT.

void Timer_Config()
{
  TIM_TimeBaseInitTypeDef TIM_TimeBaseStructure;
  NVIC_InitTypeDef NVIC_InitStructure;

  /* TIM3 clock enable */
  RCC_APB1PeriphClockCmd(RCC_APB1Periph_TIM3, ENABLE);

  TIM_TimeBaseStructure.TIM_Period = 1000;
  TIM_TimeBaseStructure.TIM_Prescaler = 42000;
  TIM_TimeBaseStructure.TIM_ClockDivision = 0;
  TIM_TimeBaseStructure.TIM_CounterMode = TIM_CounterMode_Up;
  TIM_TimeBaseInit(TIM3, &TIM_TimeBaseStructure);

  /* Enable the TIM3 gloabal Interrupt */
  NVIC_InitStructure.NVIC_IRQChannel = TIM3_IRQn;
  NVIC_InitStructure.NVIC_IRQChannelPreemptionPriority = 0;
  NVIC_InitStructure.NVIC_IRQChannelSubPriority = 1;
  NVIC_InitStructure.NVIC_IRQChannelCmd = ENABLE;
  NVIC_Init(&NVIC_InitStructure);

  TIM_ClearFlag(TIM3, TIM_FLAG_Update);
  TIM_ITConfig(TIM3, TIM_IT_Update, ENABLE);
  TIM_ClearFlag(TIM3, TIM_FLAG_Update);
}
/**
  * @brief  Starts the timeout for cmd timeout.
	* @param  val: Time in milliseconds
  * @retval None
  */
void Timer_Start(u32 val)
{
  TIM_Cmd(TIM3, DISABLE);
  Timeout = 0;
  TIM_TimeBaseInitTypeDef TIM_TimeBaseStructure;
  NVIC_InitTypeDef NVIC_InitStructure;

  /* TIM3 clock enable */
  RCC_APB1PeriphClockCmd(RCC_APB1Periph_TIM3, ENABLE);

  TIM_TimeBaseStructure.TIM_Period = val * 2;
  TIM_TimeBaseStructure.TIM_Prescaler = 42000;
  TIM_TimeBaseStructure.TIM_ClockDivision = 0;
  TIM_TimeBaseStructure.TIM_CounterMode = TIM_CounterMode_Up;
  TIM_TimeBaseInit(TIM3, &TIM_TimeBaseStructure);

  /* Enable the TIM3 gloabal Interrupt */
  NVIC_InitStructure.NVIC_IRQChannel = TIM3_IRQn;
  NVIC_InitStructure.NVIC_IRQChannelPreemptionPriority = 0;
  NVIC_InitStructure.NVIC_IRQChannelSubPriority = 1;
  NVIC_InitStructure.NVIC_IRQChannelCmd = ENABLE;
  NVIC_Init(&NVIC_InitStructure);

  TIM_ClearFlag(TIM3, TIM_FLAG_Update);
  TIM_ITConfig(TIM3, TIM_IT_Update, ENABLE);
  TIM_ClearFlag(TIM3, TIM_FLAG_Update);
  /* TIM3 enable counter */
  TIM_Cmd(TIM3, ENABLE);
}
/**
  * @brief  Stops the timeout for cmd timeout.
	* @param  None
  * @retval None
  */
void Timer_Stop()
{
  TIM_Cmd(TIM3, DISABLE);
  //	TIM_SetCounter(TIM3,0);
}
/**************************************test***************************************/
void SPI_TEST_INIT()
{
  SPI_InitTypeDef SPI_InitStructure;
  GPIO_InitTypeDef GPIO_InitStructure;
  //NVIC_InitTypeDef NVIC_InitStructure;

  /* Enable the SPI periph */
  RCC_APB1PeriphClockCmd(RCC_APB1Periph_SPI2, ENABLE);

  /* Enable SCK, MOSI, MISO and NSS GPIO clocks */
  RCC_AHB1PeriphClockCmd(RCC_AHB1Periph_GPIOB, ENABLE);

  GPIO_PinAFConfig(GPIOB, GPIO_PinSource13, GPIO_AF_SPI2);
  GPIO_PinAFConfig(GPIOB, GPIO_PinSource14, GPIO_AF_SPI2);
  GPIO_PinAFConfig(GPIOB, GPIO_PinSource15, GPIO_AF_SPI2);

  GPIO_InitStructure.GPIO_Pin = GPIO_Pin_13 | GPIO_Pin_14 | GPIO_Pin_15;
  GPIO_InitStructure.GPIO_Speed = GPIO_Speed_50MHz;
  GPIO_InitStructure.GPIO_Mode = GPIO_Mode_AF;
  GPIO_InitStructure.GPIO_OType = GPIO_OType_PP;
  GPIO_InitStructure.GPIO_PuPd = GPIO_PuPd_UP;
  GPIO_Init(GPIOB, &GPIO_InitStructure);

  /* SPI configuration -------------------------------------------------------*/
  SPI_I2S_DeInit(SPI2);
  SPI_InitStructure.SPI_Direction = SPI_Direction_2Lines_FullDuplex;
  SPI_InitStructure.SPI_Mode = SPI_Mode_Master;
  SPI_InitStructure.SPI_DataSize = SPI_DataSize_8b;
  SPI_InitStructure.SPI_CPOL = SPI_CPOL_Low;
  SPI_InitStructure.SPI_CPHA = SPI_CPHA_1Edge;
  SPI_InitStructure.SPI_NSS = SPI_NSS_Soft;
  SPI_InitStructure.SPI_BaudRatePrescaler = SPI_BaudRatePrescaler_16;
  SPI_InitStructure.SPI_FirstBit = SPI_FirstBit_MSB;
  SPI_InitStructure.SPI_CRCPolynomial = 7;
  SPI_Init(SPI2, &SPI_InitStructure);
  SPI_Cmd(SPI2, ENABLE);
  //SPI_RxFIFOThresholdConfig(SPI1, SPI_RxFIFOThreshold_QF);

  /* Configure PC10 and PC11 in output pushpull mode */
  GPIO_InitStructure.GPIO_Pin = GPIO_Pin_7 | GPIO_Pin_8 | GPIO_Pin_9;
  GPIO_InitStructure.GPIO_Mode = GPIO_Mode_OUT;
  GPIO_InitStructure.GPIO_OType = GPIO_OType_PP;
  GPIO_InitStructure.GPIO_Speed = GPIO_Speed_50MHz;
  GPIO_InitStructure.GPIO_PuPd = GPIO_PuPd_NOPULL;
  GPIO_Init(GPIOB, &GPIO_InitStructure);
}
/**************************************test***************************************/
/**
  * @brief  Initializes the ADS1299 Module.
  * @param  None
  * @retval None
  */
void ADS1299_Config()
{
  SPI_TEST_INIT();
  //  init_GPIO();
  init_SPI2();
  __enable_irq(); // Initialize IIR filters for Real Time Impedance Calculation
  for (int i = 0; i < ADS1299_CHANNELS; i++)
  {
    arm_biquad_cascade_df2T_init_f32(&biquad_HP_Struct[i], BIQUAD_STAGES_HP, biquad_HP_Coeffs, biquad_HP_State[i]);
    arm_biquad_cascade_df2T_init_f32(&biquad_BP_Struct[i], BIQUAD_STAGES_BP, biquad_BP_Coeffs, biquad_BP_State[i]);
  }
  ads1299_init();
  init_EXTI();
}

/**
  * @brief  Initializes the GPIO's.
  * @param  None
  * @retval None
  */
void GPIO_Config()
{
  initLeds();
}
/**
  * @brief  Initialize COM Port for Debugging.
	* @param  baud:	UART baudrate
  * @retval None
  */
void Debug_COM_Init()
{
  GPIO_InitTypeDef GPIO_InitStructure;
  USART_InitTypeDef USART_InitStructure;
  NVIC_InitTypeDef NVIC_InitStructure;
  /* GPIOA Periph clock enable */
  RCC_AHB1PeriphClockCmd(RCC_AHB1Periph_GPIOA, ENABLE);
  RCC_APB2PeriphClockCmd(RCC_APB2Periph_USART1, ENABLE);

  NVIC_InitStructure.NVIC_IRQChannel = USART1_IRQn;
  NVIC_InitStructure.NVIC_IRQChannelPreemptionPriority = 0;
  NVIC_InitStructure.NVIC_IRQChannelSubPriority = 1;
  NVIC_InitStructure.NVIC_IRQChannelCmd = ENABLE;
  NVIC_Init(&NVIC_InitStructure);

  GPIO_PinAFConfig(GPIOA, GPIO_PinSource9, GPIO_AF_USART1);
  GPIO_PinAFConfig(GPIOA, GPIO_PinSource10, GPIO_AF_USART1);

  GPIO_InitStructure.GPIO_Mode = GPIO_Mode_AF;
  GPIO_InitStructure.GPIO_Speed = GPIO_Speed_50MHz;
  GPIO_InitStructure.GPIO_OType = GPIO_OType_PP;
  GPIO_InitStructure.GPIO_PuPd = GPIO_PuPd_UP;
  GPIO_InitStructure.GPIO_Pin = GPIO_Pin_9 | GPIO_Pin_10;
  GPIO_Init(GPIOA, &GPIO_InitStructure);

  USART_InitStructure.USART_BaudRate = 9600;
  USART_InitStructure.USART_WordLength = USART_WordLength_8b;
  USART_InitStructure.USART_StopBits = USART_StopBits_1;
  USART_InitStructure.USART_Parity = USART_Parity_No;
  USART_InitStructure.USART_HardwareFlowControl = USART_HardwareFlowControl_None;
  USART_InitStructure.USART_Mode = USART_Mode_Rx | USART_Mode_Tx;
  USART_Init(USART1, &USART_InitStructure);
  USART_Cmd(USART1, ENABLE);
  USART_ITConfig(USART1, USART_IT_RXNE, ENABLE); // Enable USART1 Receive Interrupt
}
/**
  * @brief  Send ASCII string to Debug COM Port.
  * @param  None
  * @retval None
  */
void Debug_SendData_s(unsigned char *pucBuffer)
{
  while (USART_GetFlagStatus(DEBUG_COM, USART_FLAG_TC) == RESET)
    ;
  // Loop while there are more characters to send.
  while (*pucBuffer)
  {
    USART_SendData(DEBUG_COM, (uint16_t)*pucBuffer++);
    /* Loop until the end of transmission */
    while (USART_GetFlagStatus(DEBUG_COM, USART_FLAG_TC) == RESET)
      ;
  }
}
/**
  * @brief  Send HEXADECIMAL Bytes to Wifi Module.
  * @param  *pucBuffer: pointer to the string 
  * @retval None
  */
void Debug_SendBytes_h(uint8_t *pucBuffer, uint8_t len)
{
  while (USART_GetFlagStatus(DEBUG_COM, USART_FLAG_TC) == RESET)
    ;
  // Loop while there are more characters to send.
  for (unsigned int i = 0; i < len; i++)
  {
    USART_SendData(DEBUG_COM, (uint16_t)*pucBuffer++);
    /* Loop until the end of transmission */
    while (USART_GetFlagStatus(DEBUG_COM, USART_FLAG_TC) == RESET)
      ;
  }
}
/**
  * @brief  Write Device ID to flash.
  * @param  deviceId: Device ID 
  * @retval None
  */
void Set_Device_ID(char *deviceId)
{
  uint8_t len = strlen(deviceId);
  /* Unlock the Flash *********************************************************/
  /* Enable the flash control register access */
  FLASH_Unlock();

  /* Erase the user Flash area ************************************************/
  /* area defined by FLASH_USER_START_ADDR and FLASH_USER_END_ADDR */

  /* Clear pending flags (if any) */
  FLASH_ClearFlag(FLASH_FLAG_EOP | FLASH_FLAG_OPERR | FLASH_FLAG_WRPERR |
                  FLASH_FLAG_PGAERR | FLASH_FLAG_PGPERR | FLASH_FLAG_PGSERR);

  if (FLASH_EraseSector(ADDR_FLASH_SECTOR_5, VoltageRange_3) != FLASH_COMPLETE)
  {
    /* Error occurred while sector erase. 
         User can add here some code to deal with this error  */
    while (1)
    {
    }
  }
  /* Program the user Flash area byte ********************************/
  //  for (int i = 0; i < len; i++)
  //  {
  //    if (FLASH_ProgramByte(ADDR_FLASH_SECTOR_5 + i, deviceId[i]) == FLASH_COMPLETE)
  //    {
  //    }
  //    else
  //    {
  //      /* Error occurred while writing data in Flash memory.
  //         User can add here some code to deal with this error */
  //      while (1)
  //      {
  //      }
  //    }
  //  }
  /* Lock the Flash to disable the flash control register access (recommended
     to protect the FLASH memory against possible unwanted operation) */
  FLASH_Lock();
}
/**
  * @brief  Get Device ID from flash.
  * @param  None 
  * @retval Device ID
  */
void Get_Device_ID(char *did)
{
  did[0] = *(__IO uint32_t *)ADDR_FLASH_SECTOR_0;
}
RCC_ClocksTypeDef clocks;
RTC_TimeTypeDef RTC_TIME;
RTC_DateTypeDef RTC_DATE;
int count;
/**
  * @brief  Main program.
  * @param  None
  * @retval None
  */
int main()
{
  GPIO_Config();
  //  Timer_Config();
  NVIC_Configuration();
  ADS1299_Config();
  GSM_COM_Init(9600);
  Debug_COM_Init();
  RTC_Config();
  //  Set_Device_ID("this");
  //  Get_Device_ID(DID);

  RCC_GetClocksFreq(&clocks);
  /*------------------------------ SD Init ---------------------------------- */
  if ((Status = SD_Init()) != SD_OK)
  {
    Debug_SendData_s((unsigned char *)"******************SD CARD ERROR*****************");
  }

//  char str[80];
//  sprintf(str, "Card Capacity = %llu\r\n", SDCardInfo.CardCapacity);
//  Debug_SendData_s((unsigned char *)str);
//  
//	sprintf(str, "Card Block Size = %d\r\n", SDCardInfo.CardBlockSize);
//  Debug_SendData_s((unsigned char *)str);
      // while ((Status == SD_OK) && (uwSDCardOperation != SD_OPERATION_END) && (SD_Detect() == SD_PRESENT))
      // {
      //   switch (uwSDCardOperation)
      //   {
      //   /*-------------------------- SD Erase Test ---------------------------- */
      //   case (SD_OPERATION_ERASE):
      //   {
      //     SD_EraseTest();
      //     //			LED_3_ON;
      //     uwSDCardOperation = SD_OPERATION_BLOCK;
      //     break;
      //   }
      //   /*-------------------------- SD Single Block Test --------------------- */
      //   case (SD_OPERATION_BLOCK):
      //   {
      //     SD_SingleBlockTest();
      //     uwSDCardOperation = SD_OPERATION_MULTI_BLOCK;
      //     break;
      //   }
      //   /*-------------------------- SD Multi Blocks Test --------------------- */
      //   case (SD_OPERATION_MULTI_BLOCK):
      //   {
      // SD_MultiBlockTest();
      //     uwSDCardOperation = SD_OPERATION_END;
      //     break;
      //   }
      //   }
      // }

      Debug_SendData_s((unsigned char *)"*********************Ready*********************\r\n");
			Debug_SendData_s((unsigned char *) "*********************Conversion Started*********************\r\n");
      start_conversion();
  while (1)
  {
  char str[80];
    if (data_ready)
    {
      RTC_GetTime(RTC_Format_BIN, &RTC_TIME);
      // uint8_t hr = RTC_TIME.RTC_Hours;
      // uint8_t mn = RTC_TIME.RTC_Minutes;
      // uint8_t sc = RTC_TIME.RTC_Seconds;
      // sprintf(str, "Time = %d:%d:%d\r\n", hr, mn, sc);
      // Debug_SendData_s((unsigned char *)str);

      RTC_GetDate(RTC_Format_BIN, &RTC_DATE);

//           Status = SD_WriteMultiBlocks(aBuffer_MultiBlock_Tx, 0, BLOCK_SIZE, NUMBER_OF_BLOCKS);

      /* Check if the Transfer is finished */
      Status = SD_WaitWriteOperation();
      while (SD_GetStatus() != SD_TRANSFER_OK)
        ;
      // uint8_t dt = RTC_DATE.RTC_Date;
      // uint8_t mnt = RTC_DATE.RTC_Month;
      // uint8_t yr = RTC_DATE.RTC_Year;

      // sprintf(str, "Date = %d/%d/%d\r\n", dt, mnt, yr);
      // Debug_SendData_s((unsigned char *)str);
      // sprintf(str, "DID = %s\r\n", DID);
      // Debug_SendData_s((unsigned char *)str);
    }
    // Timer_Start(1000);
    /* Write block of 512 bytes on address 0 */
    //    Status = SD_WriteBlock(aBuffer_Block_Tx, 0x00, BLOCK_SIZE);
    //    /* Check if the Transfer is finished */
    //    Status = SD_WaitWriteOperation();
    //    while (SD_GetStatus() != SD_TRANSFER_OK);
    //		count = TIM_GetCounter(TIM3);
    //		break;
    GSM_Event();
    // Wifi_ReceiveFromServer(50);

    __DELAY(MILI_S(1000));
  }
}
/**
  * @brief  Initialize Button.
  * @param  None
  * @retval None
  */
void initBtn()
{
  GPIO_InitTypeDef GPIO_InitStructure;
  NVIC_InitTypeDef NVIC_InitStructure;
  EXTI_InitTypeDef EXTI_InitStructure;
  /* GPIO Periph clock enable */
  RCC_AHB1PeriphClockCmd(RCC_AHB1Periph_GPIOB, ENABLE);
  RCC_APB2PeriphClockCmd(RCC_APB2Periph_SYSCFG, ENABLE);

  SYSCFG_EXTILineConfig(Button_PortSource, Button_PinSource);

  GPIO_InitStructure.GPIO_Pin = Button_Pin;
  GPIO_InitStructure.GPIO_Mode = GPIO_Mode_IN;
  GPIO_InitStructure.GPIO_OType = GPIO_OType_PP;
  GPIO_InitStructure.GPIO_Speed = GPIO_Speed_50MHz;
  GPIO_InitStructure.GPIO_PuPd = GPIO_PuPd_NOPULL;
  GPIO_Init(Button_Port, &GPIO_InitStructure);

  /* Configure Button EXTI line */
  EXTI_InitStructure.EXTI_Line = Button_Line;
  EXTI_InitStructure.EXTI_Mode = EXTI_Mode_Interrupt;
  EXTI_InitStructure.EXTI_Trigger = EXTI_Trigger_Falling;
  EXTI_InitStructure.EXTI_LineCmd = ENABLE;
  EXTI_Init(&EXTI_InitStructure);

  NVIC_InitStructure.NVIC_IRQChannel = Button_IRQn;
  NVIC_InitStructure.NVIC_IRQChannelPreemptionPriority = 0;
  NVIC_InitStructure.NVIC_IRQChannelSubPriority = 1;
  NVIC_InitStructure.NVIC_IRQChannelCmd = ENABLE;
  NVIC_Init(&NVIC_InitStructure);
}
/**
  * @brief  Initialize Led's.
  * @param  None
  * @retval None
  */
void initLeds()
{
  RCC_AHB1PeriphClockCmd(RCC_AHB1Periph_GPIOC, ENABLE);

  GPIO_InitTypeDef GPIO_Init_Structure;
  GPIO_StructInit(&GPIO_Init_Structure);
  GPIO_Init_Structure.GPIO_Mode = GPIO_Mode_OUT;
  GPIO_Init_Structure.GPIO_Pin = LED_2_Pin | LED_3_Pin | LED_4_Pin;
  GPIO_Init(LED_2_Port, &GPIO_Init_Structure);

  // GPIO_SetBits(GPIOC, GPIO_Pin_0 | GPIO_Pin_2 | GPIO_Pin_4);
}
/**
  * @brief  Function for generating delay in ms.
  * @param  ms: value of the delay in milliseconds
  * @retval None
  */
void delay(uint32_t ms)
{
  ms *= 3360;
  while (ms--)
  {
    //		__NOP();
  }
}
void TIM3_IRQHandler(void)
{
  if (TIM_GetITStatus(TIM3, TIM_IT_Update) != RESET)
  {
    TIM_ClearITPendingBit(TIM3, TIM_IT_Update);
    Timeout = 1;
    TIM_Cmd(TIM3, DISABLE);
    if (ld)
    {
      LED_4_Toggle;

      Timer_Start(5000);
    }
  }
}
/**
  * @brief  IRQ Handler for Button.
  * @param  None
  * @retval None
  */
void EXTI2_IRQHandler(void)
{
  if (EXTI_GetITStatus(Button_Line) == SET)
  {
    EXTI_ClearITPendingBit(EXTI_Line2);
    if (GPIO_ReadInputDataBit(GPIOB, GPIO_Pin_2) == 0)
    {
      //			GPIO_SetBits(GSM_RST_Port, GSM_RST_Pin);
      delay(200);
      NVIC_SystemReset();
    }
  }
}

/**
  * @brief  Configures SDIO IRQ channel.
  * @param  None
  * @retval None
  */
static void NVIC_Configuration(void)
{
  NVIC_InitTypeDef NVIC_InitStructure;

  /* Configure the NVIC Preemption Priority Bits */
  NVIC_PriorityGroupConfig(NVIC_PriorityGroup_1);

  NVIC_InitStructure.NVIC_IRQChannel = SDIO_IRQn;
  NVIC_InitStructure.NVIC_IRQChannelPreemptionPriority = 0;
  NVIC_InitStructure.NVIC_IRQChannelSubPriority = 0;
  NVIC_InitStructure.NVIC_IRQChannelCmd = ENABLE;
  NVIC_Init(&NVIC_InitStructure);
  NVIC_InitStructure.NVIC_IRQChannel = SD_SDIO_DMA_IRQn;
  NVIC_InitStructure.NVIC_IRQChannelPreemptionPriority = 1;
  NVIC_Init(&NVIC_InitStructure);
}

/**
  * @brief  Tests the SD card erase operation.
  * @param  None
  * @retval None
  */
static void SD_EraseTest(void)
{
  /*------------------- Block Erase ------------------------------------------*/
  if (Status == SD_OK)
  {
    /* Erase NumberOfBlocks Blocks of WRITE_BL_LEN(512 Bytes) */
    Status = SD_Erase(0x00, (BLOCK_SIZE * NUMBER_OF_BLOCKS));
  }

  if (Status == SD_OK)
  {
    Status = SD_ReadMultiBlocks(aBuffer_MultiBlock_Rx, 0x00, BLOCK_SIZE, NUMBER_OF_BLOCKS);

    /* Check if the Transfer is finished */
    Status = SD_WaitReadOperation();

    /* Wait until end of DMA transfer */
    while (SD_GetStatus() != SD_TRANSFER_OK)
      ;
  }

  /* Check the correctness of erased blocks */
  if (Status == SD_OK)
  {
    EraseStatus = eBuffercmp(aBuffer_MultiBlock_Rx, MULTI_BUFFER_SIZE);
  }

  if (EraseStatus == PASSED)
  {
    //    STM_EVAL_LEDOn(LED1);
  }
  else
  {
    //    STM_EVAL_LEDOff(LED1);
    //    STM_EVAL_LEDOn(LED4);
  }
}

/**
  * @brief  Tests the SD card Single Blocks operations.
  * @param  None
  * @retval None
  */
static void SD_SingleBlockTest(void)
{
  /*------------------- Block Read/Write --------------------------*/
  /* Fill the buffer to send */
  Fill_Buffer(aBuffer_Block_Tx, BLOCK_SIZE, 0x320F);

  if (Status == SD_OK)
  {
    /* Write block of 512 bytes on address 0 */
    Status = SD_WriteBlock(aBuffer_Block_Tx, 0x00, BLOCK_SIZE);
    /* Check if the Transfer is finished */
    Status = SD_WaitWriteOperation();
    while (SD_GetStatus() != SD_TRANSFER_OK)
      ;
  }

  if (Status == SD_OK)
  {
    /* Read block of 512 bytes from address 0 */
    Status = SD_ReadBlock(aBuffer_Block_Rx, 0x00, BLOCK_SIZE);
    /* Check if the Transfer is finished */
    Status = SD_WaitReadOperation();
    while (SD_GetStatus() != SD_TRANSFER_OK)
      ;
  }

  /* Check the correctness of written data */
  if (Status == SD_OK)
  {
    TransferStatus1 = Buffercmp(aBuffer_Block_Tx, aBuffer_Block_Rx, BLOCK_SIZE);
  }

  if (TransferStatus1 == PASSED)
  {
    //    STM_EVAL_LEDOn(LED2);
  }
  else
  {
    //STM_EVAL_LEDOff(LED2);
    //STM_EVAL_LEDOn(LED4);
  }
}

/**
  * @brief  Tests the SD card Multiple Blocks operations.
  * @param  None
  * @retval None
  */
static void SD_MultiBlockTest(void)
{
  /* Fill the buffer to send */
  //  Fill_Buffer(aBuffer_MultiBlock_Tx, MULTI_BUFFER_SIZE, 0x0);

  if (Status == SD_OK)
  {
    /* Write multiple block of many bytes on address 0 */
    //    Status = SD_WriteMultiBlocks(aBuffer_MultiBlock_Tx, 0, BLOCK_SIZE, NUMBER_OF_BLOCKS);

    /* Check if the Transfer is finished */
    Status = SD_WaitWriteOperation();
    while (SD_GetStatus() != SD_TRANSFER_OK)
      ;
  }

  if (Status == SD_OK)
  {
    /* Read block of many bytes from address 0 */
    Status = SD_ReadMultiBlocks(aBuffer_MultiBlock_Rx, 0, BLOCK_SIZE, NUMBER_OF_BLOCKS);

    /* Check if the Transfer is finished */
    Status = SD_WaitReadOperation();
    while (SD_GetStatus() != SD_TRANSFER_OK)
      ;
  }

  /* Check the correctness of written data */
  if (Status == SD_OK)
  {
    //    TransferStatus2 = Buffercmp(aBuffer_MultiBlock_Tx, aBuffer_MultiBlock_Rx, MULTI_BUFFER_SIZE);
  }

  if (TransferStatus2 == PASSED)
  {
    //STM_EVAL_LEDOn(LED3);
  }
  else
  {
    //STM_EVAL_LEDOff(LED3);
    //STM_EVAL_LEDOn(LED4);
  }
}

/**
  * @brief  Compares two buffers.
  * @param  pBuffer1, pBuffer2: buffers to be compared.
  * @param  BufferLength: buffer's length
  * @retval PASSED: pBuffer1 identical to pBuffer2
  *         FAILED: pBuffer1 differs from pBuffer2
  */
static TestStatus Buffercmp(uint8_t *pBuffer1, uint8_t *pBuffer2, uint32_t BufferLength)
{
  while (BufferLength--)
  {
    if (*pBuffer1 != *pBuffer2)
    {
      return FAILED;
    }

    pBuffer1++;
    pBuffer2++;
  }

  return PASSED;
}

/**
  * @brief  Fills buffer with user predefined data.
  * @param  pBuffer: pointer on the Buffer to fill
  * @param  BufferLength: size of the buffer to fill
  * @param  Offset: first value to fill on the Buffer
  * @retval None
  */
static void Fill_Buffer(uint8_t *pBuffer, uint32_t BufferLength, uint32_t Offset)
{
  uint16_t index = 0;

  /* Put in global buffer same values */
  for (index = 0; index < BufferLength; index++)
  {
    pBuffer[index] = index + Offset;
  }
}

/**
  * @brief  Checks if a buffer has all its values are equal to zero.
  * @param  pBuffer: buffer to be compared.
  * @param  BufferLength: buffer's length
  * @retval PASSED: pBuffer values are zero
  *         FAILED: At least one value from pBuffer buffer is different from zero.
  */
static TestStatus eBuffercmp(uint8_t *pBuffer, uint32_t BufferLength)
{
  while (BufferLength--)
  {
    /* In some SD Cards the erased state is 0xFF, in others it's 0x00 */
    if ((*pBuffer != 0xFF) && (*pBuffer != 0x00))
    {
      return FAILED;
    }

    pBuffer++;
  }

  return PASSED;
}

unsigned char Debug_rxBuffer[1000];   //Buffer for holding data received from GSM UART.
unsigned int Debug_rxBufferIndex = 0; //Index of the GSM UART Buffer.
uint16_t Debug_rxBufferSize = 1000;   //Size of the buffer
uint16_t Debug_rxBufferLen;           //Length of the data received from GSM UART.

void USART1_IRQHandler(void)
{
  uint8_t temp;
  RTC_TimeTypeDef new_time;
  RTC_DateTypeDef new_date;
  char *date;
  char *month;
  char *year;
  if (USART_GetITStatus(USART1, USART_IT_RXNE) != RESET)
  {
    // Read one byte from the receive data register
    temp = USART_ReceiveData(USART1);
    //USART_Put(WIFI_COM,temp);
    Debug_rxBuffer[Debug_rxBufferIndex] = temp; // copy the data to rx buffer
    Debug_rxBufferIndex++;
    if (Debug_rxBuffer[Debug_rxBufferIndex - 2] == 0x0D && Debug_rxBuffer[Debug_rxBufferIndex - 1] == 0x0A) // check for valid message footer
    {
      if (Debug_rxBufferIndex < Debug_rxBufferSize) // filter the messages
      {
        Debug_rxBuffer[Debug_rxBufferIndex] = 0x00;
        Debug_rxBufferIndex = 0; // reset the buffer index

        Debug_rxBufferLen = strlen((const char *)Debug_rxBuffer);
        if (strstr((const char *)Debug_rxBuffer, (const char *)"$Set Time="))
        {
          strtok((char *)Debug_rxBuffer, "=:");
          new_time.RTC_Hours = atoi(strtok(NULL, "=:"));
          new_time.RTC_Minutes = atoi(strtok(NULL, "=:"));
          RTC_SetTime(RTC_Format_BIN, &new_time);
        }
        if (strstr((const char *)Debug_rxBuffer, (const char *)"$Set Date="))
        {
          strtok((char *)Debug_rxBuffer, "=/");
          new_date.RTC_Date = atoi(strtok(NULL, "=/"));
          new_date.RTC_Month = atoi(strtok(NULL, "=/"));
          new_date.RTC_Year = atoi(strtok(NULL, "=/"));
          RTC_SetDate(RTC_Format_BIN, &new_date);
        }
        if (strstr((const char *)Debug_rxBuffer, (const char *)"$Start"))
        {
					Debug_SendData_s((unsigned char *) "*********************Conversion Started******************\r\n");
          start_conversion();
        }
      }
      else
      {
        Debug_rxBuffer[Debug_rxBufferIndex - 1] = NULL;
        Debug_rxBuffer[Debug_rxBufferIndex - 2] = NULL;
        Debug_rxBufferIndex = 0;
      }
    }

    if (Debug_rxBufferIndex == 1000)
    {
      Debug_rxBufferIndex = 0;
    }
  }
}
#ifdef USE_FULL_ASSERT
/**
  * @brief  Reports the name of the source file and the source line number
  *         where the assert_param error has occurred.
  * @param  file: pointer to the source file name
  * @param  line: assert_param error line source number
  * @retval None
  */
void assert_failed(uint8_t *file, uint32_t line)
{
  /* User can add his own implementation to report the file name and line number,
     ex: printf("Wrong parameters value: file %s on line %d\r\n", file, line) */

  /* Infinite loop */
  while (1)
  {
  }
}
#endif