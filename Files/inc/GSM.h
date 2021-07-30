/* Define to prevent recursive inclusion -------------------------------------*/
#ifndef __GSM_H
#define __GSM_H

#ifdef __cplusplus
 extern "C" {
#endif
	 
#include "stm32f4xx.h"
#include "stm32f4xx_usart.h"
#include "MCU_IO.h"
	 
#define DEBUG_COM											USART1
	 
#define GSM_COM											  USART2
#define GSM_COM_BAUD								  9600
		                                  
#define GSM_APN		           				  "zonginternet"
#define GSM_IPv		           				  "IPv6"
			                                
#define Retry_Att				          		10
	 
#define CTRL_Z				          		  26
#define AT_Q				            		  "AT+QPOWD=1\r\n"
#define AT					            		  "AT\r\n"
#define AT_GSN			            		  "AT+GSN\r\n"
#define rn					            		  "\r\n"
#define GSM_RSSI		           			  "AT+CSQ\r\n"
#define GSM_APNQ		           			  "AT+QICSGP=1\r\n"
#define GSM_QMTCFG		          		  "AT+QMTCFG="
#define GSM_CGACT		        	  		  "AT+CGACT=1,1\r\n"
#define GSM_QIACTQ		       	  		  "AT+QIACT?\r\n"
#define GSM_IMSI		       	 	  		  "AT+CIMI\r\n"
#define GSM_CREG		       	 	  		  "AT+CREG?\r\n"
#define GSM_RST		       	 	  		    "AT+CRESET\r\n"
#define GSM_NETOPEN		       	 	  		"AT+NETOPEN\r\n"
		                                  
#define GPS_QCSQ			          		  "AT+QCSQ\r\n"
#define GPS_CFG_Q	          	  		  "AT+QGPSCFG=?\r\n"
#define GPS_QGPS_eQ             		  "AT+QGPS=?\r\n"
#define GPS_QGPS_Q              		  "AT+QGPS?\r\n"
#define GPS_ON		              		  "AT+QGPS=1\r\n"
#define GPS_CFG		          	  		  "AT+QGPSCFG=\"outport\",\"uartnmea\"\r\n"
#define GPS_QGPS		            		  "AT+QGPS=1,1,3,0,1\r\n"
#define GPS_OFF		           	  		  "AT+QGPSEND\r\n"
                        
extern uint8_t gsmCmd;

enum gsm_state {
gsm_NULL,				
gsm_Idle,							
gsm_Nregistered,  
gsm_Nsearching,							  
gsm_Nregdenied,							  
gsm_Nunknown,								  
gsm_Nroaming,								  
gsm_Nnotsearching,							
gsm_POWWERED_DOWN,     		
gsm_OK, 					
gsm_MQTTConnected, 			
gsm_MQTTDisconnected, 		
gsm_ServerConnected, 		
gsm_ServerDisconnected,		
gsm_Failed, 				
gsm_ResetCompleted,			
gsm_Ready,					
gsm_Success,				
gsm_Timeout,				
gsm_SndMsg,					
gsm_QMTPUB,			
gsm_NetOpen,
gsm_APNDone,
gsm_PDPActivated
};

	                                    
// #define gsm_Nnull					 					  0								
// #define gsm_Nregistered							  1
// #define gsm_Nsearching							  2
// #define gsm_Nregdenied							  3
// #define gsm_Nunknown								  4
// #define gsm_Nroaming								  5
// #define gsm_Nnotsearching 					  6
	
#define mqtt_C_Connected						 	0				
#define mqtt_C_UnacceptableProtocol 	1								
#define mqtt_C_IdentifierRejcted			2
#define mqtt_C_ServerUnavailable			3
#define mqtt_C_WrongUserPswd					4
#define mqtt_C_NotAuthorised					5
#define mqtt_P_Sent										6
#define mqtt_P_NotSent								7
#define mqtt_S_Sent										8
#define mqtt_S_NotSent								9

void GSM_GPS_Activate(void);
void GSM_COM_Init(u32 baud);
void GSM_SendData_s(unsigned char *pucBuffer);
void GSM_Get_IMEI(void);
void GSM_Get_RSSI(void);
void MQTT_Config();
void GSM_Read_SMS(u8 ind);
void GSM_Send_Connect_PKT();
void GSM_Send_Publish_PKT(unsigned char *tpc,unsigned char *msg);
void GSM_Send_Subscribe_PKT(unsigned char *tpc);
void GSM_Set_APN(unsigned char *apn);
void GSM_Activate_PDP_Context();
void GSM_Connect_Server();
void GSM_Set_baud(u32 baud);
void GSM_REG_STATUS(void);
void GSM_Get_baud();
void GSM_Reset(void);
void GSM_GPS_Deactivate(void);
void GSM_Open_Socket(void);
void GSM_Event(void);
#ifdef __cplusplus
}
#endif

#endif 


