#ifndef IP_GPIO_BTN_H
#define IP_GPIO_BTN_H

#include "stm32f10x.h"

// Cau hinh phan cung nut bam
#define BTN_GPIO_PORT       GPIOA
#define BTN_GPIO_PIN        GPIO_Pin_1          // PA1
#define BTN_GPIO_CLK        RCC_APB2Periph_GPIOA
#define BTN_DEBOUNCE_MS     50U                 // Thoi gian debounce (ms)

void IP_BTN_Init(void);

// 1: dang nhan, 0: khong nhan
uint8_t IP_BTN_IsPressed(void);

// Cho nut duoc NHAN roi THA hoan toan
void IP_BTN_WaitPressRelease(void);

#endif
