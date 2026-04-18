#include "MID_oled_ssd1306.h"
#include "IP_oled_font.h"
#include <string.h>

// Bo dem man hinh: 128 cot x 8 trang = 1024 bytes
uint8_t OLED_Buffer[OLED_WIDTH * OLED_PAGES];

// I2C_WaitStart - Cho den khi co dieu kien START (SR1.SB = 1)
static void I2C_WaitStart(void)
{
    uint32_t timeout = I2C_TIMEOUT;
    while (!(OLED_I2C->SR1 & I2C_SR1_SB)) {
        if (--timeout == 0) return;
    }
}

// I2C_WaitAddr - Cho den khi dia chi duoc chap nhan (SR1.ADDR = 1)
static void I2C_WaitAddr(void)
{
    uint32_t timeout = I2C_TIMEOUT;
    while (!(OLED_I2C->SR1 & I2C_SR1_ADDR)) {
        if (--timeout == 0) return;
    }
    (void)OLED_I2C->SR1;
    (void)OLED_I2C->SR2;
}

// I2C_WaitTXE - Cho den khi thanh ghi DR trong (TXE = 1)
static void I2C_WaitTXE(void)
{
    uint32_t timeout = I2C_TIMEOUT;
    while (!(OLED_I2C->SR1 & I2C_SR1_TXE)) {
        if (--timeout == 0) return;
    }
}

// I2C_WaitBTF - Cho den khi bus ranh (BTF = 1)
static void I2C_WaitBTF(void)
{
    uint32_t timeout = I2C_TIMEOUT;
    while (!(OLED_I2C->SR1 & I2C_SR1_BTF)) {
        if (--timeout == 0) return;
    }
}

// I2C_OLED_Init - Khoi dong bus I2C1 cho OLED
static void I2C_OLED_Init(void)
{
    GPIO_InitTypeDef  gpio_init;
    I2C_InitTypeDef   i2c_init;

    RCC_APB2PeriphClockCmd(OLED_GPIO_CLK, ENABLE);
    RCC_APB1PeriphClockCmd(OLED_I2C_CLK, ENABLE);

    RCC_APB1PeriphResetCmd(OLED_I2C_CLK, ENABLE);
    RCC_APB1PeriphResetCmd(OLED_I2C_CLK, DISABLE);

    gpio_init.GPIO_Pin   = OLED_SCL_PIN | OLED_SDA_PIN;
    gpio_init.GPIO_Mode  = GPIO_Mode_AF_OD;
    gpio_init.GPIO_Speed = GPIO_Speed_50MHz;
    GPIO_Init(OLED_GPIO_PORT, &gpio_init);

    i2c_init.I2C_ClockSpeed          = OLED_I2C_SPEED;
    i2c_init.I2C_Mode                = I2C_Mode_I2C;
    i2c_init.I2C_DutyCycle           = I2C_DutyCycle_2;
    i2c_init.I2C_OwnAddress1         = 0x00;
    i2c_init.I2C_Ack                 = I2C_Ack_Enable;
    i2c_init.I2C_AcknowledgedAddress = I2C_AcknowledgedAddress_7bit;
    I2C_Init(OLED_I2C, &i2c_init);

    I2C_Cmd(OLED_I2C, ENABLE);
}

// OLED_SendCommand - Gui mot byte lenh den SSD1306 qua I2C
// Trinh tu: START -> [ADDR W] -> [0x00 = Co lenh] -> [CMD] -> STOP
static void OLED_SendCommand(uint8_t cmd)
{
    I2C_GenerateSTART(OLED_I2C, ENABLE);
    I2C_WaitStart();

    I2C_Send7bitAddress(OLED_I2C, (OLED_I2C_ADDRESS << 1), I2C_Direction_Transmitter);
    I2C_WaitAddr();

    I2C_WaitTXE();
    I2C_SendData(OLED_I2C, 0x00);

    I2C_WaitTXE();
    I2C_SendData(OLED_I2C, cmd);

    I2C_WaitBTF();
    I2C_GenerateSTOP(OLED_I2C, ENABLE);
}

// OLED_SendData - Gui mot byte data don le den SSD1306 (du phong)
static void OLED_SendData(uint8_t data) __attribute__((unused));
static void OLED_SendData(uint8_t data)
{
    I2C_GenerateSTART(OLED_I2C, ENABLE);
    I2C_WaitStart();

    I2C_Send7bitAddress(OLED_I2C, (OLED_I2C_ADDRESS << 1), I2C_Direction_Transmitter);
    I2C_WaitAddr();

    I2C_WaitTXE();
    I2C_SendData(OLED_I2C, 0x40);

    I2C_WaitTXE();
    I2C_SendData(OLED_I2C, data);

    I2C_WaitBTF();
    I2C_GenerateSTOP(OLED_I2C, ENABLE);
}

// OLED_SendDataBurst - Gui nhieu byte data lien tiep trong 1 phien I2C
// Trinh tu: START -> ADDR -> [0x40] -> DATA[0]...DATA[n-1] -> STOP
static void OLED_SendDataBurst(uint8_t *buf, uint16_t len)
{
    uint16_t i;

    I2C_GenerateSTART(OLED_I2C, ENABLE);
    I2C_WaitStart();

    I2C_Send7bitAddress(OLED_I2C, (OLED_I2C_ADDRESS << 1), I2C_Direction_Transmitter);
    I2C_WaitAddr();

    I2C_WaitTXE();
    I2C_SendData(OLED_I2C, 0x40);

    for (i = 0; i < len; i++) {
        I2C_WaitTXE();
        I2C_SendData(OLED_I2C, buf[i]);
    }

    I2C_WaitBTF();
    I2C_GenerateSTOP(OLED_I2C, ENABLE);
}

void OLED_Init(void)
{
    uint32_t i;

    I2C_OLED_Init();

    for (i = 0; i < 720000UL; i++) { __NOP(); }

    OLED_SendCommand(SSD1306_DISPLAY_OFF);
    OLED_SendCommand(SSD1306_SET_DISP_CLK_DIV);
    OLED_SendCommand(0x80);
    OLED_SendCommand(SSD1306_SET_MUX_RATIO);
    OLED_SendCommand(0x3F);
    OLED_SendCommand(SSD1306_SET_DISP_OFFSET);
    OLED_SendCommand(0x00);
    OLED_SendCommand(SSD1306_SET_DISP_START_LINE | 0x00);
    OLED_SendCommand(SSD1306_CHARGE_PUMP);
    OLED_SendCommand(0x14);
    OLED_SendCommand(SSD1306_SET_MEM_ADDR);
    OLED_SendCommand(SSD1306_MEM_ADDR_HORIZ);
    OLED_SendCommand(SSD1306_SET_SEG_REMAP | 0x01);
    OLED_SendCommand(SSD1306_SET_COM_OUT_DIR | 0x08);
    OLED_SendCommand(SSD1306_SET_COM_PIN);
    OLED_SendCommand(0x12);
    OLED_SendCommand(SSD1306_SET_CONTRAST);
    OLED_SendCommand(0xCF);
    OLED_SendCommand(SSD1306_SET_PRECHARGE);
    OLED_SendCommand(0xF1);
    OLED_SendCommand(SSD1306_SET_VCOM_DESEL);
    OLED_SendCommand(0x40);
    OLED_SendCommand(SSD1306_SET_ENTIRE_ON);
    OLED_SendCommand(SSD1306_SET_NORM_INV);
    OLED_SendCommand(SSD1306_DISPLAY_ON);

    OLED_Clear();
    OLED_UpdateScreen();
}

void OLED_Clear(void)
{
    memset(OLED_Buffer, 0x00, sizeof(OLED_Buffer));
}

void OLED_Fill(uint8_t data)
{
    memset(OLED_Buffer, data, sizeof(OLED_Buffer));
}

void OLED_SetCursor(uint8_t page, uint8_t col)
{
    OLED_SendCommand(SSD1306_SET_PAGE_ADDR);
    OLED_SendCommand(page);
    OLED_SendCommand(0x07);

    OLED_SendCommand(SSD1306_SET_COL_ADDR);
    OLED_SendCommand(col);
    OLED_SendCommand(0x7F);
}

void OLED_DrawPixel(uint8_t x, uint8_t y, uint8_t color)
{
    if (x >= OLED_WIDTH || y >= OLED_HEIGHT) return;

    if (color) {
        OLED_Buffer[x + (y / 8) * OLED_WIDTH] |=  (1 << (y % 8));
    } else {
        OLED_Buffer[x + (y / 8) * OLED_WIDTH] &= ~(1 << (y % 8));
    }
}

void OLED_UpdateScreen(void)
{
    uint8_t page;

    OLED_SendCommand(SSD1306_SET_COL_ADDR);
    OLED_SendCommand(0x00);
    OLED_SendCommand(0x7F);

    OLED_SendCommand(SSD1306_SET_PAGE_ADDR);
    OLED_SendCommand(0x00);
    OLED_SendCommand(0x07);

    for (page = 0; page < OLED_PAGES; page++) {
        OLED_SendDataBurst(&OLED_Buffer[page * OLED_WIDTH], OLED_WIDTH);
    }
}

void OLED_DrawWaveform(uint16_t *samples, uint16_t num_samples)
{
    uint8_t  x;
    uint8_t  row, col;
    int16_t  sample_val;
    int16_t  y_center = (int16_t)(OLED_HEIGHT / 2);
    int16_t  y_pixel;
    uint16_t step;
    uint16_t idx;

    for (row = 8; row < OLED_HEIGHT; row++) {
        for (col = 0; col < OLED_WIDTH; col++) {
            OLED_DrawPixel(col, row, 0);
        }
    }

    for (x = 0; x < OLED_WIDTH; x++) {
        OLED_DrawPixel(x, (uint8_t)y_center, 1);
    }

    if (num_samples == 0 || samples == NULL) return;

    step = num_samples / OLED_WIDTH;
    if (step == 0) step = 1;

    for (x = 0; x < OLED_WIDTH; x++) {
        idx = x * step;
        if (idx >= num_samples) break;

        sample_val = (int16_t)samples[idx];
        y_pixel = y_center - ((sample_val - 2048) * 28 / 2048);

        if (y_pixel < 9)  y_pixel = 9;
        if (y_pixel > 63) y_pixel = 63;

        {
            int16_t yy;
            int16_t y_lo = (y_pixel >= y_center) ? y_center : y_pixel;
            int16_t y_hi = (y_pixel >= y_center) ? y_pixel  : y_center;
            for (yy = y_lo; yy <= y_hi; yy++) {
                OLED_DrawPixel(x, (uint8_t)yy, 1);
            }
        }
    }

    OLED_UpdateScreen();
}

void OLED_DrawChar(uint8_t x, uint8_t y, char c)
{
    uint8_t col;
    uint8_t row;
    uint8_t byte_val;
    uint8_t font_index;

    if (c < 0x20 || c > 0x7E) { c = 0x20; }
    font_index = (uint8_t)(c - 0x20);

    for (col = 0; col < FONT_WIDTH; col++) {
        byte_val = Font5x7[font_index][col];
        for (row = 0; row < FONT_HEIGHT; row++) {
            if (byte_val & (uint8_t)(1u << row)) {
                OLED_DrawPixel((uint8_t)(x + col), (uint8_t)(y + row), 1u);
            } else {
                OLED_DrawPixel((uint8_t)(x + col), (uint8_t)(y + row), 0u);
            }
        }
    }
}

void OLED_DrawString(uint8_t x, uint8_t y, const char *str)
{
    while (*str) {
        if (x + FONT_WIDTH >= OLED_WIDTH) break;
        OLED_DrawChar(x, y, *str);
        x += (FONT_WIDTH + 1);
        str++;
    }
}
