/* Define to prevent recursive inclusion -------------------------------------*/
#ifndef __FLASH_H
#define __FLASH_H

#ifdef __cplusplus
 extern "C" {
#endif

#include "stm32f4xx.h"
#include "MCU_IO.h"

#define DUMMY_BYTE 0XFF
	 
#define sFLASH_SPI_PAGESIZE 0x100

#define Read_ID 0x4B

#define status_1 0xD7


#define Enable_write  0x06
#define Enable_volatile_status_write 0x50
#define Disable_write 0x04

#define write_status_1 0x01
#define write_status_2 0x31
#define write_status_3 0x11

#define Read_data 0x03
#define Fast_read 0x0B

#define Page_program 0x02
#define Page_Errase

#define Sector_erase  0x20
#define Block_erase32 0x52
#define Block_erase64 0xD8




#define sFLASH_SPI                           SPI1
#define sFLASH_SPI_CLK                       RCC_APB2Periph_SPI1
#define sFLASH_SPI_CLK_INIT                  RCC_APB2PeriphClockCmd

#define sFLASH_SPI_SCK_PIN                   GPIO_Pin_5
#define sFLASH_SPI_SCK_GPIO_PORT             GPIOA
#define sFLASH_SPI_SCK_GPIO_CLK              RCC_AHB1Periph_GPIOA
#define sFLASH_SPI_SCK_SOURCE                GPIO_PinSource5
#define sFLASH_SPI_SCK_AF                    GPIO_AF_SPI1

#define sFLASH_SPI_MISO_PIN                  GPIO_Pin_6
#define sFLASH_SPI_MISO_GPIO_PORT            GPIOA
#define sFLASH_SPI_MISO_GPIO_CLK             RCC_AHB1Periph_GPIOA
#define sFLASH_SPI_MISO_SOURCE               GPIO_PinSource6
#define sFLASH_SPI_MISO_AF                   GPIO_AF_SPI1

#define sFLASH_SPI_MOSI_PIN                  GPIO_Pin_7
#define sFLASH_SPI_MOSI_GPIO_PORT            GPIOA
#define sFLASH_SPI_MOSI_GPIO_CLK             RCC_AHB1Periph_GPIOA
#define sFLASH_SPI_MOSI_SOURCE               GPIO_PinSource7
#define sFLASH_SPI_MOSI_AF                   GPIO_AF_SPI1

//#define sFLASH_CS_PIN                        GPIO_Pin_11
//#define sFLASH_CS_GPIO_PORT                  GPIOG
//#define sFLASH_CS_GPIO_CLK                   RCC_AHB1Periph_GPIOG


#define sFLASH_CS_PIN                        SPI_CS_Pin
#define sFLASH_CS_GPIO_PORT                  SPI_CS_Port
#define sFLASH_CS_GPIO_CLK                   RCC_AHB1Periph_GPIOA


#define sFLASH_RESET_PIN                     SPI_RST_Pin
#define sFLASH_RESET_GPIO_PORT               SPI_RST_Port
#define sFLASH_RESET_GPIO_CLK                RCC_AHB1Periph_GPIOB

#define sFLASH_WP_PIN                        SPI_WP_Pin
#define sFLASH_WP_GPIO_PORT                  SPI_WP_Port
#define sFLASH_WP_GPIO_CLK                   RCC_AHB1Periph_GPIOA

/* Exported macro ------------------------------------------------------------*/
/* Select sFLASH: Chip Select pin low */
#define sFLASH_CS_LOW()       GPIO_ResetBits(SPI_CS_Port, SPI_CS_Pin);
/* Deselect sFLASH: Chip Select pin high */
#define sFLASH_CS_HIGH()      GPIO_SetBits(SPI_CS_Port, SPI_CS_Pin);   



uint8_t flash_Page_erase(uint16_t page);
uint8_t Read_Byte(uint16_t page,uint16_t total_bytes,uint8_t *pdata);
uint8_t Write_FByte(uint16_t page,uint16_t total_bytes,uint8_t *buffer);
void ReadStatus(void);

void sFLASH_WriteBuffer(uint8_t* pBuffer, uint32_t WriteAddr, uint16_t NumByteToWrite,uint8_t erras_cmd);
void write_byte(uint8_t * pdata,uint8_t * writeaddr,uint16_t NumByteToWrite);
void bytes_read(uint8_t * pdata,uint32_t  address, uint16_t NumByteToRead);
void write_instruction(uint8_t instruction);
void read_ID(void);
void Erase_sector(uint8_t * mem_address);
void Erase_block32(uint8_t * mem_address);
void data_program(uint8_t * pdata,uint8_t * paddress);
void page_read(uint8_t * pdata,uint8_t * paddress);
uint8_t check_busy(void);

void update_page(uint8_t * base_address,uint8_t * pdata, uint8_t byte_address);
void read_page(uint8_t* pdata,uint8_t page_no,uint8_t sector_no,uint8_t block_no);
void page_write(uint8_t* pdata,uint8_t page_no,uint8_t sector_no,uint8_t block_no);
void page_write_without_Err(uint8_t* pdata,uint8_t page_no,uint8_t sector_no,uint8_t block_no);
void sFLASH_Init(void);



#ifdef __cplusplus
}
#endif

#endif /* __FLASH_H */


