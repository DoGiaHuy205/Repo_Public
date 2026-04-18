#include "IP_USART1.h"
#include "stm32f10x_rcc.h"
#include "stm32f10x_gpio.h"
#include "stm32f10x_usart.h"

// IP_USART1_Init
void IP_USART1_Init(uint32_t baudrate)
{
    GPIO_InitTypeDef  gpio;
    USART_InitTypeDef usart;

    // Bat clock cho GPIOA va USART1 (ca hai thuoc APB2)
    RCC_APB2PeriphClockCmd(RCC_APB2Periph_GPIOA | RCC_APB2Periph_USART1, ENABLE);

    gpio.GPIO_Pin   = GPIO_Pin_9;
    gpio.GPIO_Mode  = GPIO_Mode_AF_PP;
    gpio.GPIO_Speed = GPIO_Speed_50MHz;
    GPIO_Init(GPIOA, &gpio);

    gpio.GPIO_Pin  = GPIO_Pin_10;
    gpio.GPIO_Mode = GPIO_Mode_IN_FLOATING;
    GPIO_Init(GPIOA, &gpio);

    usart.USART_BaudRate            = baudrate;
    usart.USART_WordLength          = USART_WordLength_8b;
    usart.USART_StopBits            = USART_StopBits_1;
    usart.USART_Parity              = USART_Parity_No;
    usart.USART_HardwareFlowControl = USART_HardwareFlowControl_None;
    usart.USART_Mode                = USART_Mode_Tx | USART_Mode_Rx;
    USART_Init(USART1, &usart);

    USART_Cmd(USART1, ENABLE);
}

// IP_USART1_SendByte
void IP_USART1_SendByte(uint8_t b)
{
    while (!(USART1->SR & USART_SR_TXE));
    USART1->DR = b;
}

// IP_USART1_SendBuf
void IP_USART1_SendBuf(const uint8_t *buf, uint32_t len)
{
    uint32_t i;
    for (i = 0; i < len; i++) {
        IP_USART1_SendByte(buf[i]);
    }
}

// IP_USART1_WaitTC
void IP_USART1_WaitTC(void)
{
    while (!(USART1->SR & USART_SR_TC));
}
