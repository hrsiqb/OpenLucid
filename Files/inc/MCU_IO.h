#include "stm32f4xx.h"
#include "stm32f4xx_gpio.h"
#include "stm32f4xx_rcc.h"
#include "stm32f4xx_usart.h"
#include "misc.h"
#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include "stdio.h"
#include "stm32f4xx_exti.h"
#include "stm32f4xx_syscfg.h"
#include "stm32f4xx_spi.h"
#include "stm32f4xx_tim.h"

#define LED_2_ON		    								   GPIO_SetBits(LED_2_Port, LED_2_Pin)
#define LED_2_OFF		    								   GPIO_ResetBits(LED_2_Port, LED_2_Pin)
#define LED_2_Toggle	    								 GPIO_ToggleBits(LED_2_Port, LED_2_Pin)																				

#define LED_3_ON		    								   GPIO_SetBits(LED_3_Port, LED_3_Pin)
#define LED_3_OFF		    								   GPIO_ResetBits(LED_3_Port, LED_3_Pin)
#define LED_3_Toggle	    							       GPIO_ToggleBits(LED_3_Port, LED_3_Pin)
																							
#define LED_4_ON		    								   GPIO_SetBits(LED_4_Port, LED_4_Pin)
#define LED_4_OFF		    								   GPIO_ResetBits(LED_4_Port, LED_4_Pin)
#define LED_4_Toggle    								       GPIO_ToggleBits(LED_4_Port, LED_4_Pin)

#define TRUE												   1
#define FALSE												   0

#define SERVER_WS_DATA_GPIO									   9

#define Button_Pin			        	                       GPIO_Pin_2
#define Button_Port        	          			               GPIOB
#define Button_Line	             					           EXTI_Line2      
#define Button_IRQn	             					           EXTI2_IRQn  
#define Button_PinSource         					           EXTI_PinSource2  
#define Button_PortSource        					           EXTI_PortSourceGPIOB  

#define LED_4_Pin			        	                       GPIO_Pin_0
#define LED_3_Pin		        	                           GPIO_Pin_2
#define LED_2_Pin	        		                           GPIO_Pin_4
#define LED_4_Port        	                               GPIOC
#define LED_3_Port        	                               GPIOC
#define LED_2_Port       	                               GPIOC
                                             
#define NRF_RST_Pin			        	                       GPIO_Pin_13
#define NRF_AT_MODE_Pin	        	                           GPIO_Pin_12
#define NRF_RST_Port		        	                       GPIOE
#define NRF_AT_MODE_Port        	                           GPIOB
                                             
// #define GSM_RST_Pin			        	                       GPIO_Pin_1
// #define GSM_PWR_Key_Pin	        	                           GPIO_Pin_0
#define GSM_PWR_Pin			        	                       GPIO_Pin_4
// #define GSM_STATUS_Pin	        	                           GPIO_Pin_3
// #define GSM_RST_Port		        	                       GPIOE
// #define GSM_PWR_Key_Port        	                           GPIOE
#define GSM_PWR_Port		        	                       GPIOA
                                             
#define PERIPH_PWR_Pin	        	                           GPIO_Pin_4
#define WB_PWR_Pin			        	                       GPIO_Pin_5
#define PERIPH_PWR_Port	        	                           GPIOE
#define WB_PWR_Port			        	                       GPIOE
                                             
#define SPI_CS_Pin	            	                           GPIO_Pin_4
#define SPI_WP_Pin			        	                       GPIO_Pin_8
#define SPI_RST_Pin			        	                       GPIO_Pin_0
#define SPI_RST_Port	                                       GPIOB
#define SPI_CS_Port	            	                           GPIOA
#define SPI_WP_Port	            	                           GPIOA
                                             
#define WIFI_RST_Pin		        	                       GPIO_Pin_12
#define WIFI_server_data_Pin			                       GPIO_Pin_15
#define WIFI_RST_Port			                               GPIOE
#define WIFI_server_data_Port	                               GPIOA
#define WIFI_server_data_Line	                               EXTI_Line15      
#define WIFI_server_data_IRQn	                               EXTI15_10_IRQn  
#define WIFI_server_data_PinSource                             EXTI_PinSource15  
#define WIFI_server_data_PortSource                            EXTI_PortSourceGPIOA  
                                             
#define SI4460_nSEL_Pin		                                   GPIO_Pin_8
#define SI4460_IRQ_Pin		                                   GPIO_Pin_1
#define SI4460_IRQ_Port		                                   GPIOB
#define SI4460_nSEL_Port	                                   GPIOB
                                             
#define STX3_RST_Pin			                               GPIO_Pin_0
#define STX3_Test1_Pin		                                   GPIO_Pin_4
#define STX3_Test2_Pin		                                   GPIO_Pin_5
#define STX3_Test1_Port		                                   GPIOC
#define STX3_Test2_Port		                                   GPIOC
#define STX3_RST_Port			                               GPIOC

