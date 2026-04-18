#include "IP_GPIO_BTN.h"
#include "IP_SYSCLK.h"
#include "stm32f10x_rcc.h"
#include "stm32f10x_gpio.h"

// IP_BTN_Init - Cau hinh PA1 = Input Pull-Up
void IP_BTN_Init(void)
{
    GPIO_InitTypeDef gpio;

    // Bat clock GPIOA (APB2)
    RCC_APB2PeriphClockCmd(BTN_GPIO_CLK, ENABLE);

    // PA1: Input Pull-Up
    gpio.GPIO_Pin   = BTN_GPIO_PIN;
    gpio.GPIO_Mode  = GPIO_Mode_IPU;
    gpio.GPIO_Speed = GPIO_Speed_2MHz;
    GPIO_Init(BTN_GPIO_PORT, &gpio);
}

// IP_BTN_IsPressed
uint8_t IP_BTN_IsPressed(void)
{
    if (GPIO_ReadInputDataBit(BTN_GPIO_PORT, BTN_GPIO_PIN) == Bit_RESET) {
        // PA1 = 0: co the dang nhan -> doi debounce
        IP_Delay_ms(BTN_DEBOUNCE_MS);
        // Doc lai sau debounce
        if (GPIO_ReadInputDataBit(BTN_GPIO_PORT, BTN_GPIO_PIN) == Bit_RESET) {
            return 1U;
        }
    }
    return 0U;
}

// IP_BTN_WaitPressRelease
void IP_BTN_WaitPressRelease(void)
{
    // Cho nhan: doi PA1 xuong 0
    while (!IP_BTN_IsPressed()) { __NOP(); }

    // Cho tha: doi PA1 len 1
    while (GPIO_ReadInputDataBit(BTN_GPIO_PORT, BTN_GPIO_PIN) == Bit_RESET) {
        __NOP();
    }

    // Debounce khi tha de tranh bat toa
    IP_Delay_ms(BTN_DEBOUNCE_MS);
}
