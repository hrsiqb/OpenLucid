/* Define to prevent recursive inclusion -------------------------------------*/
#ifndef __MQTT_H
#define __MQTT_H

#ifdef __cplusplus
 extern "C" {
#endif
	 
#include "stm32f4xx.h"
	 
#define SERVER_IP								"broker.hivemq.com"
#define SERVER_PORT							"1883"
	 
//#define SERVER_IP								"192.168.100.21"
//#define SERVER_PORT							"23"
	 
#define MQTT_USER								"bmyateiw"
#define MQTT_PSWD								"hWp2ye65SWBK"
#define MQTT_CID								"ABCDEF"
	 
#define TOPIC										"STX-3"
#define TOPIC_DATA							"hello"
	 
#define WIFI_SSID   						"opticalconnect"
#define WIFI_PSWD 							"optical54321"
	
//#define WIFI_SSID   						"bridge"
//#define WIFI_PSWD 							"12345789"
	 
uint32_t charToUInt32(const char* src);
void Get_ServerData(void);
void Send_ConnectPacket(void);
void Send_PublishPacket(unsigned char *topc,unsigned char *sdata);
void Send_SubscribePacket(unsigned char *topc);
#ifdef __cplusplus
}
#endif

#endif 


