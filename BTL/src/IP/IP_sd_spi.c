#include "IP_sd_spi.h"
#include "stm32f10x_spi.h"
#include "stm32f10x_gpio.h"
#include "stm32f10x_rcc.h"
#include <string.h>

// Bien noi bo: loai the da duoc khoi dong
static uint8_t sd_card_type = SD_TYPE_UNKNOWN;

// Macro dieu khien CS
#define SD_CS_LOW()     GPIO_ResetBits(SD_GPIO_PORT, SD_CS_PIN)
#define SD_CS_HIGH()    GPIO_SetBits(SD_GPIO_PORT, SD_CS_PIN)

// SPI_Xchg - Trao doi 1 byte qua SPI1 (Full Duplex)
static uint8_t SPI_Xchg(uint8_t dat)
{
    uint32_t timeout = 500000UL;
    // Doi TXE (data register trong)
    while (!(SD_SPI_PORT->SR & SPI_SR_TXE)) {
        if (--timeout == 0) return 0xFF;
    }
    SD_SPI_PORT->DR = dat;

    timeout = 500000UL;
    // Doi RXNE (du lieu da nhan)
    while (!(SD_SPI_PORT->SR & SPI_SR_RXNE)) {
        if (--timeout == 0) return 0xFF;
    }
    return (uint8_t)(SD_SPI_PORT->DR);
}

// SPI_ReadByte - Gui byte 0xFF (dat duong) de doc 1 byte tu SD
static uint8_t SPI_ReadByte(void)
{
    return SPI_Xchg(0xFF);
}

// SPI_SetSpeed - Cau hinh lai prescaler SPI (thay doi toc do)
static void SPI_SetSpeed(uint16_t prescaler)
{
    uint16_t tmp;
    // Tat SPI truoc khi thay doi BR bits
    SD_SPI_PORT->CR1 &= ~SPI_CR1_SPE;
    tmp = (uint16_t)(SD_SPI_PORT->CR1 & ~SPI_CR1_BR);
    SD_SPI_PORT->CR1 = tmp | prescaler | SPI_CR1_SPE;
}

// SPI1_Init_SD - Khoi tao SPI1 GPIO va SPI peripheral
static void SPI1_Init_SD(void)
{
    GPIO_InitTypeDef  gpio;
    SPI_InitTypeDef   spi;

    // Bat clock
    RCC_APB2PeriphClockCmd(SD_GPIO_CLK | SD_SPI_CLK, ENABLE);

    // PA4 = CS: Output Push-Pull
    gpio.GPIO_Pin   = SD_CS_PIN;
    gpio.GPIO_Mode  = GPIO_Mode_Out_PP;
    gpio.GPIO_Speed = GPIO_Speed_50MHz;
    GPIO_Init(SD_GPIO_PORT, &gpio);

    // PA5=SCK, PA7=MOSI: Alternate Function Push-Pull
    gpio.GPIO_Pin  = SD_SCK_PIN | SD_MOSI_PIN;
    gpio.GPIO_Mode = GPIO_Mode_AF_PP;
    GPIO_Init(SD_GPIO_PORT, &gpio);

    // PA6=MISO: Input Floating
    gpio.GPIO_Pin  = SD_MISO_PIN;
    gpio.GPIO_Mode = GPIO_Mode_IN_FLOATING;
    GPIO_Init(SD_GPIO_PORT, &gpio);

    // CS HIGH (giai phong the)
    SD_CS_HIGH();

    // Cau hinh SPI1:
    spi.SPI_Mode              = SPI_Mode_Master;
    spi.SPI_Direction         = SPI_Direction_2Lines_FullDuplex;
    spi.SPI_DataSize          = SPI_DataSize_8b;
    spi.SPI_CPOL              = SPI_CPOL_Low;
    spi.SPI_CPHA              = SPI_CPHA_1Edge;
    spi.SPI_NSS               = SPI_NSS_Soft;
    spi.SPI_BaudRatePrescaler = SPI_BaudRatePrescaler_256;
    spi.SPI_FirstBit          = SPI_FirstBit_MSB;
    spi.SPI_CRCPolynomial     = 7;
    SPI_Init(SD_SPI_PORT, &spi);

    // Bat SPI
    SPI_Cmd(SD_SPI_PORT, ENABLE);
}

// SD_WaitReady - Cho den khi SD khong con busy (MISO = 0xFF)
static uint8_t SD_WaitReady(void)
{
    uint32_t timeout = 500000UL;
    while (SPI_ReadByte() != 0xFF) {
        if (--timeout == 0) return 0;
    }
    return 1;
}

// SD_SendCmd - Gui lenh SPI SD 6 byte
static uint8_t SD_SendCmd(uint8_t cmd, uint32_t arg, uint8_t crc)
{
    uint8_t  r1;
    uint32_t timeout;

    // Doi SD san sang (MISO = 0xFF)
    if (!SD_WaitReady()) return 0xFF;

    // Gui 6 byte lenh
    SPI_Xchg(cmd);
    SPI_Xchg((uint8_t)(arg >> 24));
    SPI_Xchg((uint8_t)(arg >> 16));
    SPI_Xchg((uint8_t)(arg >> 8));
    SPI_Xchg((uint8_t)(arg));
    SPI_Xchg(crc);

    // Doi phan hoi R1 (toi da 8 lan thu)
    timeout = 8;
    do {
        r1 = SPI_ReadByte();
        timeout--;
    } while ((r1 & 0x80U) && timeout);

    return r1;
}

// SD_SPI_Init - Khoi tao SPI1 va the SD card
uint8_t SD_SPI_Init(void)
{
    uint8_t  r1;
    uint8_t  buf[4];
    uint32_t timeout;
    uint32_t i;

    sd_card_type = SD_TYPE_UNKNOWN;

    // Khoi tao phan cung SPI1
    SPI1_Init_SD();

    // Gui >= 74 xung clock voi CS=HIGH de the chuyen sang SPI mode
    SD_CS_HIGH();
    for (i = 0; i < 10U; i++) {
        SPI_ReadByte(); // 10 bytes = 80 xung
    }

    // Buoc 1: CMD0 - Dat lai the SD
    SD_CS_LOW();
    r1 = SD_SendCmd(SD_CMD0, 0x00000000UL, 0x95U); // CRC=0x95 cho CMD0
    SD_CS_HIGH();
    SPI_ReadByte(); // 1 dummy byte sau moi lenh

    if (r1 != SD_R1_IDLE) {
        return SD_ERR_NO_CARD; // Khong co the hoac khong phan hoi
    }

    // Buoc 2: CMD8 - Xac dinh phien ban SD
    SD_CS_LOW();
    r1 = SD_SendCmd(SD_CMD8, 0x000001AAUL, 0x87U); // CRC=0x87 cho CMD8
    if (r1 == SD_R1_IDLE) {
        // SDv2: doc 4 byte phan hoi R7
        buf[0] = SPI_ReadByte();
        buf[1] = SPI_ReadByte();
        buf[2] = SPI_ReadByte();
        buf[3] = SPI_ReadByte();
        SD_CS_HIGH();
        SPI_ReadByte();

        if (buf[3] != 0xAAU) {
            return SD_ERR_INIT_FAIL; // Dien ap khong tuong thich
        }
        sd_card_type = SD_TYPE_SDv2; // Co the la SDHC
    } else {
        // SDv1 hoac MMC: khong ho tro CMD8
        SD_CS_HIGH();
        SPI_ReadByte();
        sd_card_type = SD_TYPE_SDv1;
    }

    // Buoc 3: ACMD41 - Kich hoat the SD
    timeout = 100000UL;
    do {
        // CMD55 (APP_CMD)
        SD_CS_LOW();
        SD_SendCmd(SD_CMD55, 0x00000000UL, 0xFFU);
        SD_CS_HIGH();
        SPI_ReadByte();

        // ACMD41
        SD_CS_LOW();
        if (sd_card_type == SD_TYPE_SDv2) {
            r1 = SD_SendCmd(SD_ACMD41, 0x40000000UL, 0xFFU); // HCS=1
        } else {
            r1 = SD_SendCmd(SD_ACMD41, 0x00000000UL, 0xFFU);
        }
        SD_CS_HIGH();
        SPI_ReadByte();

        timeout--;
    } while ((r1 != SD_R1_OK) && timeout);

    if (r1 != SD_R1_OK) {
        return SD_ERR_INIT_FAIL;
    }

    // Buoc 4: CMD58 - Kiem tra SDHC hay SDv2 binh thuong
    if (sd_card_type == SD_TYPE_SDv2) {
        SD_CS_LOW();
        r1 = SD_SendCmd(SD_CMD58, 0x00000000UL, 0xFFU);
        buf[0] = SPI_ReadByte();
        buf[1] = SPI_ReadByte();
        buf[2] = SPI_ReadByte();
        buf[3] = SPI_ReadByte();
        SD_CS_HIGH();
        SPI_ReadByte();

        if ((r1 == SD_R1_OK) && (buf[0] & 0x40U)) {
            sd_card_type = SD_TYPE_SDHC; // CCS bit = 1 -> SDHC
        }
    }

    // Buoc 5: CMD16 - Dat block size = 512 cho SDv1/SDv2
    if (sd_card_type != SD_TYPE_SDHC) {
        SD_CS_LOW();
        r1 = SD_SendCmd(SD_CMD16, 512UL, 0xFFU);
        SD_CS_HIGH();
        SPI_ReadByte();
        if (r1 != SD_R1_OK) {
            return SD_ERR_INIT_FAIL;
        }
    }

    // Buoc 6: Tang toc do SPI len cao (72MHz/8 = 9MHz)
    SPI_SetSpeed(SPI_BaudRatePrescaler_8);

    return SD_OK;
}

// SD_GetType - Tra ve loai the SD da khoi dong
uint8_t SD_GetType(void)
{
    return sd_card_type;
}

// SD_ReadBlock - Doc 1 block 512 byte tu the SD
uint8_t SD_ReadBlock(uint32_t block_addr, uint8_t *buf)
{
    uint8_t  token;
    uint32_t timeout;
    uint32_t i;

    // SDv1/SDv2: chuyen block address sang byte address
    if (sd_card_type != SD_TYPE_SDHC) {
        block_addr <<= 9; // * 512
    }

    SD_CS_LOW();

    // Gui CMD17 (READ_SINGLE_BLOCK)
    if (SD_SendCmd(SD_CMD17, block_addr, 0xFFU) != SD_R1_OK) {
        SD_CS_HIGH();
        SPI_ReadByte();
        return SD_ERR_READ;
    }

    // Cho token bat dau du lieu (0xFE), toi da ~100ms
    timeout = 200000UL;
    do {
        token = SPI_ReadByte();
        timeout--;
    } while ((token == 0xFF) && timeout);

    if (token != SD_TOKEN_START_BLOCK) {
        SD_CS_HIGH();
        SPI_ReadByte();
        return SD_ERR_READ;
    }

    // Doc 512 byte du lieu
    for (i = 0; i < 512U; i++) {
        buf[i] = SPI_ReadByte();
    }

    // Doc 2 byte CRC (bo qua)
    SPI_ReadByte();
    SPI_ReadByte();

    SD_CS_HIGH();
    SPI_ReadByte();

    return SD_OK;
}

// SD_WriteBlock - Ghi 1 block 512 byte vao the SD
uint8_t SD_WriteBlock(uint32_t block_addr, const uint8_t *buf)
{
    uint8_t  resp;
    uint32_t i;

    // SDv1/SDv2: chuyen block address sang byte address
    if (sd_card_type != SD_TYPE_SDHC) {
        block_addr <<= 9;
    }

    SD_CS_LOW();

    // Gui CMD24 (WRITE_BLOCK)
    if (SD_SendCmd(SD_CMD24, block_addr, 0xFFU) != SD_R1_OK) {
        SD_CS_HIGH();
        SPI_ReadByte();
        return SD_ERR_WRITE;
    }

    // Gui 1 byte dummy + token bat dau du lieu
    SPI_ReadByte();
    SPI_Xchg(SD_TOKEN_WRITE_START);

    // Ghi 512 byte du lieu
    for (i = 0; i < 512U; i++) {
        SPI_Xchg(buf[i]);
    }

    // Gui 2 byte CRC gia (khong su dung CRC)
    SPI_Xchg(0xFFU);
    SPI_Xchg(0xFFU);

    // Kiem tra data response token
    resp = SPI_ReadByte();
    if ((resp & 0x1FU) != 0x05U) {
        SD_CS_HIGH();
        SPI_ReadByte();
        return SD_ERR_WRITE;
    }

    // Cho the ghi xong (MISO = 0xFF)
    if (!SD_WaitReady()) {
        SD_CS_HIGH();
        SPI_ReadByte();
        return SD_ERR_WRITE;
    }

    SD_CS_HIGH();
    SPI_ReadByte();

    return SD_OK;
}
