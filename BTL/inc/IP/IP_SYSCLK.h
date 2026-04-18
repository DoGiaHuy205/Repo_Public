#ifndef IP_SYSCLK_H
#define IP_SYSCLK_H

#include "stm32f10x.h"

// Cau hinh clock
#define SYSCLK_HZ       72000000UL  // 72 MHz
#define APB1_CLK_HZ     36000000UL  // 36 MHz
#define APB2_CLK_HZ     72000000UL  // 72 MHz

// Cau hinh System Clock 72MHz tu HSE + PLL
void IP_SystemClock_Config(void);

// Tre thoi gian busy-wait (ms mili-giay, tham chieu 72MHz)
void IP_Delay_ms(uint32_t ms);

#endif
