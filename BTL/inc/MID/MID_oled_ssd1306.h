#ifndef MID_OLED_SSD1306_H
#define MID_OLED_SSD1306_H

#include "stm32f10x.h"

// I2C1: SCL = PB6, SDA = PB7, dia chi SSD1306: 0x3C (7-bit)
#define OLED_I2C                I2C1
#define OLED_I2C_CLK            RCC_APB1Periph_I2C1
#define OLED_GPIO_CLK           RCC_APB2Periph_GPIOB
#define OLED_GPIO_PORT          GPIOB
#define OLED_SCL_PIN            GPIO_Pin_6
#define OLED_SDA_PIN            GPIO_Pin_7

#define OLED_I2C_ADDRESS        0x3C    // Dia chi 7-bit
#define OLED_I2C_SPEED          400000UL

// Kich thuoc man hinh
#define OLED_WIDTH              128
#define OLED_HEIGHT             64
#define OLED_PAGES              8

// Timeout cho I2C
#define I2C_TIMEOUT             10000

// Tap lenh SSD1306
#define SSD1306_DISPLAY_OFF         0xAE
#define SSD1306_DISPLAY_ON          0xAF
#define SSD1306_SET_CONTRAST        0x81
#define SSD1306_SET_ENTIRE_ON       0xA4
#define SSD1306_SET_NORM_INV        0xA6
#define SSD1306_SET_DISP_OFFSET     0xD3
#define SSD1306_SET_COM_PIN         0xDA
#define SSD1306_SET_MEM_ADDR        0x20
#define SSD1306_MEM_ADDR_HORIZ      0x00
#define SSD1306_MEM_ADDR_VERT       0x01
#define SSD1306_MEM_ADDR_PAGE       0x02
#define SSD1306_SET_COL_ADDR        0x21
#define SSD1306_SET_PAGE_ADDR       0x22
#define SSD1306_SET_DISP_START_LINE 0x40
#define SSD1306_SET_SEG_REMAP       0xA0
#define SSD1306_SET_MUX_RATIO       0xA8
#define SSD1306_SET_COM_OUT_DIR     0xC0
#define SSD1306_SET_DISP_CLK_DIV    0xD5
#define SSD1306_SET_PRECHARGE       0xD9
#define SSD1306_SET_VCOM_DESEL      0xDB
#define SSD1306_CHARGE_PUMP         0x8D

// Ham API chuc nang
void OLED_Init(void);
void OLED_Clear(void);
void OLED_Fill(uint8_t data);
void OLED_SetCursor(uint8_t page, uint8_t col);
void OLED_DrawPixel(uint8_t x, uint8_t y, uint8_t color);
void OLED_UpdateScreen(void);
void OLED_DrawWaveform(uint16_t *samples, uint16_t num_samples);
void OLED_DrawChar(uint8_t x, uint8_t y, char c);
void OLED_DrawString(uint8_t x, uint8_t y, const char *str);

// Buffer man hinh
extern uint8_t OLED_Buffer[OLED_WIDTH * OLED_PAGES];

#endif
