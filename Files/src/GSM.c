#include "GSM.h"
#include "main.h"

unsigned char GSM_rxBuffer[1000];	//Buffer for holding data received from GSM UART.
unsigned int GSM_rxBufferIndex = 0; //Index of the GSM UART Buffer.
uint16_t GSM_rxBufferSize = 1000;	//Size of the buffer
uint16_t GSM_rxBufferLen;			//Length of the data received from GSM UART.

extern volatile uint8_t Timeout; //The TIM3 IRQ changes this flag to 1, so that the program does'nt get stuck in a loop.
extern uint8_t Count;			 //The value of Retries done.

uint8_t gsmCmd = 0;	   //The type of command received from GSM UART.
uint8_t gsmNtwk = 0;   //The Status of GSM Network.ie. Connected,Searching etc.
uint8_t gsm_ready = 0; //Flag to check if the UART of the GSM module is ready for communication.
uint8_t get_imei = 0;  //Flag to check if the imei is ready to be read.

enum cmd_type
{
	cmd_null,
	cmd_APN,
	cmd_PDP
} cmd_type;

struct _GSM
{								 //Structure for holding GSM Parameters.
	uint8_t IMEI[30];			 //IMEI of the GSM Module.
	uint8_t RSSI[10];			 //Signal strength of the network.
	uint8_t Server_Connect_Flag; //Server connection status.ie. Connected,NotConnected etc..
	uint8_t MQTT_Connect_Flag;	 //MQTT Connectio Status.ie.Connected,NotConnected,WrongUserPswd etc.
} GSM;

/**
  * @brief  Send ASCII string to GSM Module.
  * @param  None
  * @retval None
  */
void GSM_SendData_s(unsigned char *pucBuffer)
{
	while (USART_GetFlagStatus(GSM_COM, USART_FLAG_TC) == RESET)
		;
	// Loop while there are more characters to send.
	while (*pucBuffer)
	{
		USART_SendData(GSM_COM, (uint16_t)*pucBuffer++);
		/* Loop until the end of transmission */
		while (USART_GetFlagStatus(GSM_COM, USART_FLAG_TC) == RESET)
			;
	}
}

/**
  * @brief  Initialize COM Port for GSM.
	* @param  baud:	UART baudrate
  * @retval None
  */
void GSM_COM_Init(u32 baud)
{
	GPIO_InitTypeDef GPIO_InitStructure;
	USART_InitTypeDef USART_InitStructure;
	NVIC_InitTypeDef NVIC_InitStructure;
	/* GPIOA Periph clock enable */
	RCC_AHB1PeriphClockCmd(RCC_AHB1Periph_GPIOA, ENABLE);
	// RCC_AHB1PeriphClockCmd(RCC_AHB1Periph_GPIOE, ENABLE);
	// RCC_AHB1PeriphClockCmd(RCC_AHB1Periph_GPIOC, ENABLE);
	RCC_APB1PeriphClockCmd(RCC_APB1Periph_USART2, ENABLE);

	NVIC_InitStructure.NVIC_IRQChannel = USART2_IRQn;
	NVIC_InitStructure.NVIC_IRQChannelPreemptionPriority = 0;
	NVIC_InitStructure.NVIC_IRQChannelSubPriority = 1;
	NVIC_InitStructure.NVIC_IRQChannelCmd = ENABLE;
	NVIC_Init(&NVIC_InitStructure);

	GPIO_InitStructure.GPIO_Pin = GSM_PWR_Pin;
	GPIO_InitStructure.GPIO_Mode = GPIO_Mode_OUT;
	GPIO_InitStructure.GPIO_OType = GPIO_OType_PP;
	GPIO_InitStructure.GPIO_Speed = GPIO_Speed_50MHz;
	GPIO_InitStructure.GPIO_PuPd = GPIO_PuPd_NOPULL;
	GPIO_Init(GSM_PWR_Port, &GPIO_InitStructure);

	GPIO_PinAFConfig(GPIOA, GPIO_PinSource2, GPIO_AF_USART2);
	GPIO_PinAFConfig(GPIOA, GPIO_PinSource3, GPIO_AF_USART2);

	GPIO_InitStructure.GPIO_Mode = GPIO_Mode_AF;
	GPIO_InitStructure.GPIO_Speed = GPIO_Speed_50MHz;
	GPIO_InitStructure.GPIO_OType = GPIO_OType_PP;
	GPIO_InitStructure.GPIO_PuPd = GPIO_PuPd_UP;
	GPIO_InitStructure.GPIO_Pin = GPIO_Pin_2 | GPIO_Pin_3;
	GPIO_Init(GPIOA, &GPIO_InitStructure);

	USART_InitStructure.USART_BaudRate = baud;
	USART_InitStructure.USART_WordLength = USART_WordLength_8b;
	USART_InitStructure.USART_StopBits = USART_StopBits_1;
	USART_InitStructure.USART_Parity = USART_Parity_No;
	USART_InitStructure.USART_HardwareFlowControl = USART_HardwareFlowControl_None;
	USART_InitStructure.USART_Mode = USART_Mode_Rx | USART_Mode_Tx;
	USART_Init(USART2, &USART_InitStructure);
	USART_Cmd(USART2, ENABLE);
	USART_ITConfig(USART2, USART_IT_RXNE, ENABLE); // Enable USART2 Receive Interrupt
}
/**
  * @brief  Activates the GSM Module.
  * @param  None
  * @retval None
  */
//void GSM_GPS_Activate(void)
//{
//	GPIO_SetBits(GSM_PWR_Key_Port, GSM_PWR_Key_Pin);
//	delay(100);
//	GPIO_ResetBits(GSM_PWR_Key_Port, GSM_PWR_Key_Pin);
//	delay(5000);
//}
/**
  * @brief  Deactivates the GSM Module.
  * @param  None
  * @retval None
  */
//void GSM_GPS_Deactivate(void)
//{
//	GPIO_SetBits(GSM_PWR_Key_Port, GSM_PWR_Key_Pin);
//	delay(650);
//	GPIO_ResetBits(GSM_PWR_Key_Port, GSM_PWR_Key_Pin);
//	delay(2000);
//}

/**
  * @brief  Resets the GSM Module.
  * @param  None
  * @retval None
  */
void GSM_Reset(void)
{
	Timer_Start(30000);
	GSM_SendData_s((unsigned char *)GSM_RST);
	Timeout = 0;
	while (gsmCmd != gsm_Ready && !Timeout)
		;
	Timer_Stop();
	// GSM_GPS_Deactivate();
	// GSM_COM_Init(115200);
	// GSM_GPS_Activate();
	// GSM_Set_baud(GSM_COM_BAUD);
	// delay(1000);
	// GSM_COM_Init(GSM_COM_BAUD);
}
/**
  * @brief  Set the baudrate of GSM Module.
	* @param  baud:	New baudrate of the module
  * @retval None
  */
void GSM_Set_baud(u32 baud)
{
	uint8_t temp_str[50];

	sprintf((char *)temp_str, "AT+IPR=%d\r\n", baud);
	Timer_Start(2000);
	GSM_SendData_s((unsigned char *)temp_str);
	while (gsmCmd != gsm_OK && !Timeout)
		;
	Timer_Stop();
	gsmCmd = gsm_NULL;
}
/**
  * @brief  Get the baudrate of GSM Module.
	* @param  None
  * @retval None
  */
void GSM_Get_baud()
{
	uint8_t temp_str[50];

	sprintf((char *)temp_str, "AT+IPR?\r\n");
	Timer_Start(300);
	GSM_SendData_s((unsigned char *)temp_str);
	while (gsmCmd != gsm_OK && !Timeout)
		;
	Timer_Stop();
	gsmCmd = gsm_NULL;
}
/**
  * @brief  Reads the SMS.
	* @param  ind: index of the sms to be read
  * @retval None
  */
void GSM_Read_SMS(u8 ind)
{
	uint8_t temp_str[50];

	sprintf((char *)temp_str, "AT+CMGR=%d\r\n", ind);
	Timer_Start(300);
	GSM_SendData_s((unsigned char *)temp_str);
	while (gsmCmd != gsm_OK && !Timeout)
		;
	Timer_Stop();
	gsmCmd = gsm_NULL;
}
/**
  * @brief  Connect to MQTT Server.
	* @param  None
  * @retval None
  */
void GSM_Connect_Server()
{
	uint8_t temp_str[50];
	Count = 0;
Here:
	sprintf((char *)temp_str, "AT+CIPOPEN=1,\"TCP\",\"%s\",%s\r\n", SERVER_IP, SERVER_PORT);
	Timer_Start(300);
	GSM_SendData_s((unsigned char *)temp_str);
	while (gsmCmd != gsm_OK && !Timeout)
		;
	Timer_Stop();
	Timer_Start(5000);
	while (!Timeout)
		;
	if (GSM.Server_Connect_Flag != TRUE && (Count++) < Retry_Att)
		goto Here;
	if (Count >= Retry_Att)
		Count = 0;
	Timer_Stop();
}
/**
  * @brief  Send Connect Packet to MQTT Server.
	* @param  None
  * @retval None
  */
void GSM_Send_Connect_PKT()
{
	uint8_t temp_str[50];

	sprintf((char *)temp_str, "AT+QMTCONN=1,\"%s\",\"%s\",\"%s\"\r\n", MQTT_CID, MQTT_USER, MQTT_PSWD);
	Timer_Start(300);
	GSM_SendData_s((unsigned char *)temp_str);
	while (gsmCmd != gsm_OK && !Timeout)
		;
	Timer_Stop();
	Timer_Start(5000);
	while (GSM.MQTT_Connect_Flag != mqtt_C_Connected && !Timeout)
		;
	Timer_Stop();
	gsmCmd = gsm_NULL;
}
/**
  * @brief  Send Publish Packet to MQTT Server.
	* @param  tpc: Topic the msg is being sent to
	* @param  msg: Message to be sent
  * @retval None
  */
void GSM_Send_Publish_PKT(unsigned char *tpc, unsigned char *msg)
{
	uint8_t temp_str[50];

	sprintf((char *)temp_str, "AT+QMTPUB=1,0,0,0,\"%s\"\r\n", tpc);
	Timer_Start(300);
	GSM_SendData_s(temp_str);
	while (gsmCmd != gsm_SndMsg && !Timeout)
		;
	gsmCmd = gsm_NULL;
	sprintf((char *)temp_str, "%s%c", msg, CTRL_Z);
	Timer_Start(300);
	GSM_SendData_s(temp_str);
	while (gsmCmd != gsm_OK && !Timeout)
		;
	Timer_Start(5000);
	while (GSM.MQTT_Connect_Flag != mqtt_P_Sent && !Timeout)
		;
	Timer_Stop();
	gsmCmd = gsm_NULL;
}
/**
  * @brief  Send Subscribe Packet to MQTT Server.
	* @param  tpc: Topic that needs to be subscribed
  * @retval None
  */
void GSM_Send_Subscribe_PKT(unsigned char *tpc)
{
	uint8_t temp_str[50];

	sprintf((char *)temp_str, "AT+QMTSUB=1,1,\"%s\",0\r\n", tpc);
	Timer_Start(300);
	GSM_SendData_s(temp_str);
	while (gsmCmd != gsm_OK && !Timeout)
		;
	Timer_Start(5000);
	while (GSM.MQTT_Connect_Flag != mqtt_S_Sent && !Timeout)
		;
	Timer_Stop();
	gsmCmd = gsm_NULL;
}
/*AT+QMTOPEN=1,"m16.cloudmqtt.com",17816
OK

+QMTOPEN: 1,0
AT+QMTCONN=1,"ABCDEF","bmyateiw","hWp2ye65SWBK"
OK

+QMTCONN: 1,0,0
AT+QMTPUB=?
+QMTPUB: (0-5), <msgid>,(0-2),(0,1),"topic",[(1-1548)]

OK
AT+QMTPUB=1,0,0,0,"STX-3"
> 
OK

+QMTPUB: 1,0,0
AT+QMTPUB=1,0,0,0,"STX-3"
> hi
OK

+QMTPUB: 1,0,0
AT+QMTSUB=1,1,"STX-3",0
OK

+QMTSUB: 1,1,0,0

+QMTRECV: 1,0,"STX-3","hey there"*/
/**
  * @brief  Configure MQTT.
	* @param  ssid: ssid of the network
  * @param  pswd: passkey of the network
  * @retval None
  */
void MQTT_Config()
{
	uint8_t temp_str[50];

	GSM_SendData_s((unsigned char *)GSM_APN);
	while (gsmCmd != gsm_OK)
		;
	gsmCmd = gsm_NULL;
	delay(5000);
	//	GSM_SendData_s((unsigned char*)GSM_QIACT);
	while (gsmCmd != gsm_OK)
		;
	gsmCmd = gsm_NULL;
	GSM_SendData_s((unsigned char *)GSM_QIACTQ);
	while (gsmCmd != gsm_OK)
		;
	gsmCmd = gsm_NULL;
	/*	sprintf((char*)temp_str, "%s\"version\",1,3\r\n",GSM_QMTCFG);
	GSM_SendData_s((unsigned char*)temp_str);
	while(gsmCmd!=gsm_OK);
	
	sprintf((char*)temp_str, "%s\"pdpcid\",1,1\r\n",GSM_QMTCFG);
	GSM_SendData_s((unsigned char*)temp_str);
	while(gsmCmd!=gsm_OK);
	
	sprintf((char*)temp_str, "%s\"will\",1,0,0,0,STX-3,discon\r\n",GSM_QMTCFG);
	GSM_SendData_s((unsigned char*)temp_str);
	while(gsmCmd!=gsm_OK);
	
	sprintf((char*)temp_str, "%s\"timeout\",1,5,10,1\r\n",GSM_QMTCFG);
	GSM_SendData_s((unsigned char*)temp_str);
	while(gsmCmd!=gsm_OK);
	
	sprintf((char*)temp_str, "%s\"session\",1,0\r\n",GSM_QMTCFG);
	GSM_SendData_s((unsigned char*)temp_str);
	while(gsmCmd!=gsm_OK);
	
	sprintf((char*)temp_str, "%s\"keepalive\",1,120\r\n",GSM_QMTCFG);
	GSM_SendData_s((unsigned char*)temp_str);
	while(gsmCmd!=gsm_OK);
	
	sprintf((char*)temp_str, "%s\"ssl\",1,120\r\n",GSM_QMTCFG);
	GSM_SendData_s((unsigned char*)temp_str);
	while(gsmCmd!=gsm_OK);
	*/
	//	sprintf((char*)temp_str, "%s1,192.168.100.21,23\r\n",GSM_QMTOPN);
	//	GSM_SendData_s((unsigned char*)temp_str);
	//	while(gsmCmd!=gsm_OK);
	//	gsmCmd=gsm_NULL;
}
/**
  * @brief  Function to handle GSM Configuration
	* @param  None
  * @retval None
  */
void GSM_Event()
{
	uint8_t temp_str[100];
	switch (gsmCmd)
	{
	case gsm_NULL:
		gsmCmd = gsm_Idle;
		GSM_SendData_s((unsigned char *)"");
		delay(10);
		GSM_SendData_s((unsigned char *)GSM_RST);
		break;
	case gsm_Ready:
//		LED_4_ON;
		gsmCmd = gsm_Idle;
		GSM_SendData_s((unsigned char *)GSM_CREG);
		break;
	case gsm_Nregistered:
		gsmCmd = gsm_Idle;
		GSM_SendData_s((unsigned char *)GSM_NETOPEN);
		break;
	case gsm_NetOpen:
		gsmCmd = gsm_Idle;
		cmd_type = cmd_APN;
		sprintf((char *)temp_str, "AT+CGDCONT=1,\"IPV4V6\",\"zonginternet\",\"0.0.0.0\",0,0,0,0\r\n");
		GSM_SendData_s((unsigned char *)temp_str);
		break;
	case gsm_APNDone:
		gsmCmd = gsm_Idle;
		cmd_type = cmd_PDP;
		GSM_SendData_s((unsigned char *)GSM_CGACT);
		break;
	case gsm_PDPActivated:
		gsmCmd = gsm_Idle;
		sprintf((char *)temp_str, "AT+CIPOPEN=1,\"TCP\",\"%s\",%s\r\n", SERVER_IP, SERVER_PORT);
		GSM_SendData_s(temp_str);
		break;
	case gsm_ServerConnected:
		gsmCmd = gsm_Idle;
		GSM_SendData_s((unsigned char *)"AT+CIPSEND=1,10\r\n");
		break;
	case gsm_SndMsg:
		gsmCmd = gsm_Idle;
		GSM_SendData_s((unsigned char *)"Hello World\r\n");
		break;
	}
}
/**
  * @brief  Open Internet Socket
	* @param  None
  * @retval None
  */
void GSM_Open_Socket()
{
	Timer_Start(300);
	GSM_SendData_s((unsigned char *)GSM_NETOPEN);
	while (gsmCmd != gsm_NetOpen && !Timeout)
		;
	Timer_Stop();
	gsmCmd = gsm_NULL;
}
/**
  * @brief  Set the APN and IPv
	* @param  apn: Access Point Name
	* @param  ipv: Internet Protocol version
  * @retval None
  */
void GSM_Set_APN(unsigned char *apn)
{
	uint8_t temp_str[100];

	sprintf((char *)temp_str, "AT+CGDCONT=1,\"IPV4V6\",\"zonginternet\",\"0.0.0.0\",0,0,0,0\r\n");
	Timer_Start(300);
	GSM_SendData_s(temp_str);
	while (gsmCmd != gsm_OK && !Timeout)
		;
	Timer_Stop();
	gsmCmd = gsm_NULL;
}
/**
  * @brief  Activates the PDP Context
	* @param  None
  * @retval None
  */
void GSM_Activate_PDP_Context()
{
	Timer_Start(5000);
	GSM_SendData_s((unsigned char *)GSM_CGACT);
	while (gsmCmd != gsm_OK && !Timeout)
		;
	Timer_Stop();
	gsmCmd = gsm_NULL;
}
/**
  * @brief  Gets the IMEI of GSM Module.
  * @param  None
  * @retval None
  */
void GSM_Get_IMEI(void)
{
	Timer_Start(500);
	GSM_SendData_s((unsigned char *)AT_GSN);
	while (gsmCmd != gsm_OK && !Timeout)
		;
	Timer_Stop();
	gsmCmd = gsm_NULL;
}
/**
  * @brief  Gets the IMEI of GSM Module.
  * @param  None
  * @retval None
  */
void GSM_REG_STATUS(void)
{
	Timer_Start(300);
	GSM_SendData_s((unsigned char *)GSM_CREG);
	while (gsmCmd != gsm_OK && !Timeout)
		;
	Timer_Stop();
	gsmCmd = gsm_NULL;
}
/**
  * @brief  Gets the signal strength of GSM Module.
  * @param  None
  * @retval None
  */
void GSM_Get_RSSI(void)
{
	Timer_Start(300);
	GSM_SendData_s((unsigned char *)GSM_RSSI);
	while (gsmCmd != gsm_OK && !Timeout)
		;
	Timer_Stop();
	gsmCmd = gsm_NULL;
}

void USART2_IRQHandler(void)
{
	uint8_t temp;
	char *fst, *scnd;
	u16 len;
	if (USART_GetITStatus(USART2, USART_IT_RXNE) != RESET)
	{
		// Read one byte from the receive data register
		temp = USART_ReceiveData(USART2);
		//USART_Put(WIFI_COM,temp);
		GSM_rxBuffer[GSM_rxBufferIndex] = temp; // copy the data to rx buffer
		GSM_rxBufferIndex++;
		if (strstr((const char *)GSM_rxBuffer, (const char *)">"))
		{
			gsmCmd = gsm_SndMsg;
			GSM_rxBufferIndex = 0;
		}
		if (GSM_rxBuffer[GSM_rxBufferIndex - 2] == 0x0D && GSM_rxBuffer[GSM_rxBufferIndex - 1] == 0x0A) // check for valid message footer
		{
			if (GSM_rxBufferIndex < GSM_rxBufferSize) // filter the messages
			{
				GSM_rxBuffer[GSM_rxBufferIndex] = 0x00;
				GSM_rxBufferIndex = 0; // reset the buffer index

				GSM_rxBufferLen = strlen((const char *)GSM_rxBuffer);
				if (get_imei)
				{
					get_imei = 0;
					strcpy((char *)GSM.IMEI, (char *)GSM_rxBuffer);
				}
				if (strstr((const char *)GSM_rxBuffer, (const char *)"POWERED DOWN"))
				{
					gsmCmd = gsm_NULL;
					gsmNtwk = gsm_ServerDisconnected;
				}

				else if (strstr((const char *)GSM_rxBuffer, (const char *)"+QGPS"))
				{
				}
				else if (strstr((const char *)GSM_rxBuffer, (const char *)"RDY") ||
						 strstr((const char *)GSM_rxBuffer, (const char *)"+CPIN: READY"))
				{
					if (!gsm_ready)
						gsmCmd = gsm_Ready;
					gsm_ready = 1;
				}
				else if (strstr((const char *)GSM_rxBuffer, (const char *)"+CME ERROR: 501"))
				{
					GPIO_SetBits(GPIOD, GPIO_Pin_14);
				}

				else if (strstr((const char *)GSM_rxBuffer, (const char *)"AT+GSN"))
				{
					get_imei = 1;
				}

				else if (strstr((const char *)GSM_rxBuffer, (const char *)"OK"))
				{
					switch (cmd_type)
					{
					case cmd_APN:
						gsmCmd = gsm_APNDone;
						break;
					case cmd_PDP:
						gsmCmd = gsm_PDPActivated;
						break;
					default:
						// gsmCmd=gsm_OK;
						break;
					}
					cmd_type = cmd_null;
				}

				else if (strstr((const char *)GSM_rxBuffer, (const char *)"+QMTPUB:"))
				{
					fst = strstr((const char *)GSM_rxBuffer, (const char *)",") + 1;
					scnd = strstr((const char *)fst, (const char *)",") + 1;
					if (*scnd == '0')
						GSM.MQTT_Connect_Flag = mqtt_P_Sent;
					else if (*scnd == '2')
						GSM.MQTT_Connect_Flag = mqtt_P_NotSent;
				}
				else if (strstr((const char *)GSM_rxBuffer, (const char *)"+QMTSUB:"))
				{
					fst = strstr((const char *)GSM_rxBuffer, (const char *)",") + 1;
					scnd = strstr((const char *)fst, (const char *)",") + 1;
					if (*scnd == '0')
						GSM.MQTT_Connect_Flag = mqtt_S_Sent;
					else if (*scnd == '2')
						GSM.MQTT_Connect_Flag = mqtt_S_NotSent;
				}
				else if (strstr((const char *)GSM_rxBuffer, (const char *)"+QMTCONN:"))
				{
					fst = strstr((const char *)GSM_rxBuffer, (const char *)",") + 1;
					scnd = strstr((const char *)fst, (const char *)",") + 1;
					if (*fst == '0')
					{
						switch (*scnd)
						{
						case '0':
							GSM.MQTT_Connect_Flag = mqtt_C_Connected;
						case '1':
							GSM.MQTT_Connect_Flag = mqtt_C_UnacceptableProtocol;
						case '2':
							GSM.MQTT_Connect_Flag = mqtt_C_IdentifierRejcted;
						case '3':
							GSM.MQTT_Connect_Flag = mqtt_C_ServerUnavailable;
						case '4':
							GSM.MQTT_Connect_Flag = mqtt_C_WrongUserPswd;
						case '5':
							GSM.MQTT_Connect_Flag = mqtt_C_NotAuthorised;
						}
					}
				}
				else if (strstr((const char *)GSM_rxBuffer, (const char *)"+CIPOPEN:"))
				{
					LED_4_ON;
					fst = strstr((const char *)GSM_rxBuffer, (const char *)",") + 1;
					if (*fst == '0')
					{
						GSM.Server_Connect_Flag = TRUE;
						gsmCmd = gsm_ServerConnected;
					}
					else
					{
						GSM.Server_Connect_Flag = FALSE;
						gsmCmd = gsm_PDPActivated;
					}
				}
				else if (strstr((const char *)GSM_rxBuffer, (const char *)"+IPCLOSE:"))
				{
					LED_4_OFF;
					fst = strstr((const char *)GSM_rxBuffer, (const char *)",") + 1;
					scnd = strstr((const char *)GSM_rxBuffer, (const char *)",") + 2;
					GSM.Server_Connect_Flag = FALSE;
					gsmCmd = gsm_PDPActivated;
				}
				else if (strstr((const char *)GSM_rxBuffer, (const char *)"+QMTRECV"))
				{
				}
				else if (strstr((const char *)GSM_rxBuffer, (const char *)"+NETOPEN:"))
				{
					gsmCmd = gsm_NetOpen;
				}
				else if (strstr((const char *)GSM_rxBuffer, (const char *)"+CSQ:"))
				{
					fst = strstr((const char *)GSM_rxBuffer, (const char *)":") + 2;
					scnd = strstr((const char *)fst, (const char *)",");
					len = scnd - fst;
					strncpy((char *)GSM.RSSI, fst, len);
				}
				else if (strstr((const char *)GSM_rxBuffer, (const char *)"+CREG:"))
				{
					fst = strstr((const char *)GSM_rxBuffer, (const char *)",") + 1;
					switch (*fst)
					{
					case '0':
						gsmNtwk = gsm_Nnotsearching;
						gsmCmd = gsm_Ready;
						break;
					case '1':
						gsmNtwk = gsm_Nregistered;
						gsmCmd = gsm_Nregistered;
						break;
					case '2':
						gsmNtwk = gsm_Nsearching;
						gsmCmd = gsm_Ready;
						break;
					case '3':
						gsmNtwk = gsm_Nregdenied;
						gsmCmd = gsm_Ready;
						break;
					case '4':
						gsmNtwk = gsm_Nunknown;
						gsmCmd = gsm_Ready;
						break;
					case '5':
						gsmNtwk = gsm_Nroaming;
						gsmCmd = gsm_Nregistered;
						break;
					default:
						gsmCmd = gsm_Ready;
						break;
					}
				}
			}
			else
			{
				GSM_rxBuffer[GSM_rxBufferIndex - 1] = NULL;
				GSM_rxBuffer[GSM_rxBufferIndex - 2] = NULL;
				GSM_rxBufferIndex = 0;
			}
		}

		if (GSM_rxBufferIndex == 1000)
		{
			GSM_rxBufferIndex = 0;
		}
	}
}