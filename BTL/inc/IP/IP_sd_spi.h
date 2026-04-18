#ifndef IP_SD_SPI_H
#define IP_SD_SPI_H

#include "stm32f10x.h"

// Cau hinh phan cung SPI1
#define SD_SPI_PORT         SPI1
#define SD_SPI_CLK          RCC_APB2Periph_SPI1
#define SD_GPIO_CLK         RCC_APB2Periph_GPIOA
#define SD_GPIO_PORT        GPIOA

#define SD_CS_PIN           GPIO_Pin_4  // PA4 = CS
#define SD_SCK_PIN          GPIO_Pin_5  // PA5 = SCK
#define SD_MISO_PIN         GPIO_Pin_6  // PA6 = MISO
#define SD_MOSI_PIN         GPIO_Pin_7  // PA7 = MOSI

// Ma lenh SD (SPI Mode)
#define SD_CMD0             0x40U
#define SD_CMD8             0x48U
#define SD_CMD9             0x49U
#define SD_CMD16            0x50U
#define SD_CMD17            0x51U
#define SD_CMD24            0x58U
#define SD_CMD55            0x77U
#define SD_CMD58            0x7AU
#define SD_ACMD41           0x69U

// Token du lieu
#define SD_TOKEN_START_BLOCK    0xFEU
#define SD_TOKEN_WRITE_START    0xFEU

// Ma phan hoi R1
#define SD_R1_IDLE          0x01U
#define SD_R1_OK            0x00U

// Ket qua hoat dong
#define SD_OK               0U
#define SD_ERR_TIMEOUT      1U
#define SD_ERR_NO_CARD      2U
#define SD_ERR_INIT_FAIL    3U
#define SD_ERR_READ         4U
#define SD_ERR_WRITE        5U

// Loai the SD
#define SD_TYPE_UNKNOWN     0U
#define SD_TYPE_SDv1        1U
#define SD_TYPE_SDv2        2U
#define SD_TYPE_SDHC        3U  // SDHC/SDXC

// Khoi tao SPI1 va SD card
uint8_t SD_SPI_Init(void);

// Lay loai the (sau khi init)
uint8_t SD_GetType(void);

// Doc 1 block 512 byte tu the SD (block_addr: so block voi SDHC, byte addr voi SDv1/v2)
uint8_t SD_ReadBlock(uint32_t block_addr, uint8_t *buf);

// Ghi 1 block 512 byte vao the SD
uint8_t SD_WriteBlock(uint32_t block_addr, const uint8_t *buf);

#endif
