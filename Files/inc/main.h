#include "stm32f4xx.h"
#include "stm32f4xx_gpio.h"
#include "stm32f4xx_rcc.h"
#include "stm32f4xx_usart.h"
#include "misc.h"
#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include "stdio.h"
#include "stm32f4xx_exti.h"
#include "stm32f4xx_syscfg.h"
#include "stm32f4xx_sdio.h"
#include "stm32f4xx_spi.h"
#include "stm32f4xx_tim.h"
#include "stm32f4xx_rtc.h"
#include "stm32f4xx_pwr.h"
#include "stm32f4xx_flash.h"
#include "spi.h"
#include "MQTT.h"
#include "GSM.h"
#include "RTC.h"
#include "stm324xg_eval_sdio_sd.h"

#include "arm_math.h"
#include "arm_const_structs.h"

#define DID                 						 "000001"

#define ADS1299_CHANNELS                 8
#define BUFFERSIZE                       512

#define SD_LED_ON()                      GPIO_SetBits(SD_LED_GPIO_PORT, SD_LED_PIN)
#define SD_LED_OFF()                     GPIO_ResetBits(SD_LED_GPIO_PORT, SD_LED_PIN)

#define SD_SPI                           SPI1
#define SD_SPI_CLK                       RCC_APB2Periph_SPI1

#define SD_SPI_SCK_PIN                   GPIO_Pin_3                  /* PA.05 */
#define SD_SPI_SCK_GPIO_PORT             GPIOB                       /* GPIOA */
#define SD_SPI_SCK_GPIO_CLK              RCC_AHBPeriph_GPIOB
#define SD_SPI_SCK_SOURCE                GPIO_PinSource3
#define SD_SPI_SCK_AF                    GPIO_AF_SPI1

#define SD_SPI_MISO_PIN                  GPIO_Pin_4                  /* PA.6 */
#define SD_SPI_MISO_GPIO_PORT            GPIOB                       /* GPIOA */
#define SD_SPI_MISO_GPIO_CLK             RCC_AHBPeriph_GPIOB
#define SD_SPI_MISO_SOURCE               GPIO_PinSource4
#define SD_SPI_MISO_AF                   GPIO_AF_SPI1

#define SD_SPI_MOSI_PIN                  GPIO_Pin_5                  /* PA.7 */
#define SD_SPI_MOSI_GPIO_PORT            GPIOB                       /* GPIOA */
#define SD_SPI_MOSI_GPIO_CLK             RCC_AHBPeriph_GPIOB
#define SD_SPI_MOSI_SOURCE               GPIO_PinSource5
#define SD_SPI_MOSI_AF                   GPIO_AF_SPI1

#define SD_CS_PIN                        GPIO_Pin_2                  /* PB.01 */
#define SD_CS_GPIO_PORT                  GPIOB                       /* GPIOB */
#define SD_CS_GPIO_CLK                   RCC_AHBPeriph_GPIOB

#define GPS_GSM_PWR_ON		      		 GPIO_SetBits(GSM_PWR_Port, GSM_PWR_Pin)
#define GPS_GSM_PWR_OFF		      		 GPIO_ResetBits(GSM_PWR_Port, WGSM_PWR_Pin)
				                                 
#define CON_WIFI						 0					
#define CON_GSM							 1
#define CON_SATT						 2

/* Base address of the Flash sectors */ 
#define ADDR_FLASH_SECTOR_0     ((uint32_t)0x08000000) /* Base address of Sector 0, 16 Kbytes   */
#define ADDR_FLASH_SECTOR_1     ((uint32_t)0x08004000) /* Base address of Sector 1, 16 Kbytes   */
#define ADDR_FLASH_SECTOR_2     ((uint32_t)0x08008000) /* Base address of Sector 2, 16 Kbytes   */
#define ADDR_FLASH_SECTOR_3     ((uint32_t)0x0800C000) /* Base address of Sector 3, 16 Kbytes   */
#define ADDR_FLASH_SECTOR_4     ((uint32_t)0x08010000) /* Base address of Sector 4, 64 Kbytes   */
#define ADDR_FLASH_SECTOR_5     ((uint32_t)0x08020000) /* Base address of Sector 5, 128 Kbytes  */
#define ADDR_FLASH_SECTOR_6     ((uint32_t)0x08040000) /* Base address of Sector 6, 128 Kbytes  */
#define ADDR_FLASH_SECTOR_7     ((uint32_t)0x08060000) /* Base address of Sector 7, 128 Kbytes  */
#define ADDR_FLASH_SECTOR_8     ((uint32_t)0x08080000) /* Base address of Sector 8, 128 Kbytes  */
#define ADDR_FLASH_SECTOR_9     ((uint32_t)0x080A0000) /* Base address of Sector 9, 128 Kbytes  */
#define ADDR_FLASH_SECTOR_10    ((uint32_t)0x080C0000) /* Base address of Sector 10, 128 Kbytes */
#define ADDR_FLASH_SECTOR_11    ((uint32_t)0x080E0000) /* Base address of Sector 11, 128 Kbytes */


void init();
void loop();
void delay();
void initLeds();
void init_SPI2();
void initButton();
void initSpi(void);
void init_GPIO();
void Flash_Init(void);
void GPIO_Config();
void initLeds();			
void initBtn();
void Timer_Start(u32 val);
void Timer_Stop();
u8 Flash_SendByte(u8 byte);
u8 Flash_GetByte(void);
void Flash_Write(u8 REG, u8 *DATA, u8 count);
void Flash_Read(u8 REG, u8 *DATA, u8 count);
void Flash_ReadACC(int32_t* out);
void Flash_ReadACCY(int32_t* out);
void Debug_SendBytes_h(uint8_t *pucBuffer, uint8_t len);
void Debug_SendData_s(unsigned char *pucBuffer);

// Function Prototypes
    // Initialization Routines
    void init_SPI1();
		void init_EXTI();
    // Delays
    void __DELAY(uint32_t cycles);
    // External ASM functions
//    extern void STX(const char *X0);
//    extern void SRX(char *X0);
//    extern void CTX(char X0);
//    extern char CRX();
//    extern void PREG(uint32_t R0);
//    extern void PDEC(uint32_t R0);
		// Analysis Functions & Definitions & Variables
    #define ADS1299_SIGNAL_WINDOW 200

    // IIR Structures
    #define BIQUAD_STAGES_HP 39
    #define BIQUAD_STAGES_BP 21
   extern arm_biquad_cascade_df2T_instance_f32 biquad_HP_Struct[ADS1299_CHANNELS];
   extern float32_t biquad_HP_State[ADS1299_CHANNELS][2 * BIQUAD_STAGES_HP];
   extern float32_t biquad_HP_Coeffs[5 * BIQUAD_STAGES_HP];
   extern const float32_t biquad_HP_Output_Gain;

   extern arm_biquad_cascade_df2T_instance_f32 biquad_BP_Struct[ADS1299_CHANNELS];
   extern float32_t biquad_BP_State[ADS1299_CHANNELS][2 * BIQUAD_STAGES_BP];
   extern float32_t biquad_BP_Coeffs[5 * BIQUAD_STAGES_BP];
   extern const float32_t biquad_BP_Output_Gain;

    // BioEXG Settings variable + Function
    extern uint32_t BIOEXG_SETTINGS;
    void setting_mode();

    // Data counter
    extern uint32_t counterData;

// BIOEXG_SETTINGS Bit field:
// [24:16 - IMPEDANCE] [7:0 - CHANNELS]
    #define SETTINGS_BIT_CHANNEL(x)  1 << (x + 0)
    #define SETTINGS_BIT_IMP(x)      1 << (x + 12)

    // ADS1299 Functions
    void ads1299_init();
		void start_conversion();
		void stop_conversion();
    void ads1299_pwr_up_seq();
    void ads1299_read_data(uint32_t *STATUS, int32_t *DATA);
    void ads1299_stop_dataread();
    uint8_t ads1299_read_reg(uint8_t ADDR);
    void ads1299_write_reg(uint8_t ADDR, uint8_t VAL);
    uint8_t SPI_TX(uint8_t DATA);
    uint8_t SPI_NO_DELAY_TX(uint8_t DATA);


// Macro Definitions
#define MILI_S(x)  x * 168000/4
#define MICRO_S(x) x * 168/4

// Thanks to **OpenBCI** for the definitions :D

// SPI Command Definitions (Datasheet, 35)
#define _WAKEUP  0x02 // Wake-up from standby mode
#define _STANDBY 0x04 // Enter Standby mode
#define _RESET   0x06 // Reset the device registers to default
#define _START   0x08 // Start and restart (synchronize) conversions
#define _STOP    0x0A // Stop conversion
#define _RDATAC  0x10 // Enable Read Data Continuous mode (default mode at power-up)
#define _SDATAC  0x11 // Stop Read Data Continuous mode
#define _RDATA   0x12 // Read data by command; supports multiple read back
#define _RREG    0x20 // Read Register
#define _WREG    0x40 // Write to Register

// Register Addresses
#define ID         0x00
#define CONFIG1    0x01
#define CONFIG2    0x02
#define CONFIG3    0x03
#define LOFF       0x04
#define CH1SET     0x05
#define CH2SET     0x06
#define CH3SET     0x07
#define CH4SET     0x08
#define CH5SET     0x09
#define CH6SET     0x0A
#define CH7SET     0x0B
#define CH8SET     0x0C
#define BIAS_SENSP 0x0D
#define BIAS_SENSN 0x0E
#define LOFF_SENSP 0x0F
#define LOFF_SENSN 0x10
#define LOFF_FLIP  0x11
#define LOFF_STATP 0x12
#define LOFF_STATN 0x13
#define GPIO       0x14
#define MISC1      0x15
#define MISC2      0x16
#define CONFIG4    0x17

// Gains
#define ADS1299_PGA_GAIN01 0x00
#define ADS1299_PGA_GAIN02 0x10
#define ADS1299_PGA_GAIN04 0x20
#define ADS1299_PGA_GAIN06 0x30
#define ADS1299_PGA_GAIN08 0x40
#define ADS1299_PGA_GAIN12 0x50
#define ADS1299_PGA_GAIN24 0x60

// Input Modes - Channels

#define ADS1299_INPUT_PWR_DOWN   0x80
#define ADS1299_INPUT_PWR_UP     0x00

#define ADS1299_INPUT_NORMAL     0x00
#define ADS1299_INPUT_SHORTED    0x01
#define ADS1299_INPUT_MEAS_BIAS  0x02
#define ADS1299_INPUT_SUPPLY     0x03
#define ADS1299_INPUT_TEMP       0x04
#define ADS1299_INPUT_TESTSIGNAL 0x05
#define ADS1299_INPUT_SET_BIASP  0x06
#define ADS1299_INPUT_SET_BIASN  0x07

// Test Signal Choices - p41
#define ADS1299_TEST_INT              0x10
#define ADS1299_TESTSIGNAL_AMP_1X     0x00
#define ADS1299_TESTSIGNAL_AMP_2X     0x04
#define ADS1299_TESTSIGNAL_PULSE_SLOW 0x00
#define ADS1299_TESTSIGNAL_PULSE_FAST 0x01
#define ADS1299_TESTSIGNAL_DCSIG      0x03
#define ADS1299_TESTSIGNAL_NOCHANGE   0xff

//Lead-off Signal Choices
#define LOFF_MAG_6NA 									0x00
#define LOFF_MAG_24NA 								0x04
#define LOFF_MAG_6UA 									0x08
#define LOFF_MAG_24UA 								0x0C
#define LOFF_FREQ_DC 									0x00
#define LOFF_FREQ_7p8HZ 							0x01
#define LOFF_FREQ_31p2HZ 							0x02
#define LOFF_FREQ_FS_4 								0x03
#define PCHAN 												1
#define NCHAN 												2
#define BOTHCHAN 											3

#define OFF 													0
#define ON 														1

