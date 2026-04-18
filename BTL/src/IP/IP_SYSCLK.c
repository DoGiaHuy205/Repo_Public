#include "IP_SYSCLK.h"
#include "stm32f10x_rcc.h"
#include "stm32f10x_flash.h"

// IP_SystemClock_Config - 72MHz tu HSE 8MHz + PLL x9
void IP_SystemClock_Config(void)
{
    // Dat lai RCC ve trang thai mac dinh
    RCC_DeInit();

    // Bat HSE (thach anh ngoai 8MHz)
    RCC_HSEConfig(RCC_HSE_ON);
    if (RCC_WaitForHSEStartUp() != SUCCESS) return;

    // Flash: 2 wait states (bat buoc khi HCLK > 48MHz)
    FLASH_SetLatency(FLASH_Latency_2);
    FLASH_PrefetchBufferCmd(FLASH_PrefetchBuffer_Enable);

    // AHB  Prescaler = 1  (HCLK  = SYSCLK = 72MHz)
    // APB2 Prescaler = 1  (PCLK2 = HCLK   = 72MHz)
    // APB1 Prescaler = 2  (PCLK1 = HCLK/2 = 36MHz)
    RCC_HCLKConfig(RCC_SYSCLK_Div1);
    RCC_PCLK2Config(RCC_HCLK_Div1);
    RCC_PCLK1Config(RCC_HCLK_Div2);

    // PLL Source = HSE (khong chia), Multiplier = x9
    // SYSCLK = 8MHz x 9 = 72MHz
    RCC_PLLConfig(RCC_PLLSource_HSE_Div1, RCC_PLLMul_9);
    RCC_PLLCmd(ENABLE);

    // Doi PLL khoa (PLLRDY = 1)
    while (RCC_GetFlagStatus(RCC_FLAG_PLLRDY) == RESET);

    // Chuyen SYSCLK sang PLL (SW = 10)
    RCC_SYSCLKConfig(RCC_SYSCLKSource_PLLCLK);
    while (RCC_GetSYSCLKSource() != 0x08);

    // Cap nhat bien SystemCoreClock toan cuc cua CMSIS
    SystemCoreClockUpdate();
}

// IP_Delay_ms - Busy-wait delay dua tren 72MHz clock
void IP_Delay_ms(uint32_t ms)
{
    volatile uint32_t d = ms * 7200UL;
    while (d--) { __NOP(); }
}
