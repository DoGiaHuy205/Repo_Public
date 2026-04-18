#ifndef IP_USART1_H
#define IP_USART1_H

#include "stm32f10x.h"

// Cau hinh phan cung
#define USART1_BAUDRATE     115200UL  // Toc do baudrate

// Khoi tao USART1: baud rate
void IP_USART1_Init(uint32_t baudrate);

// Gui 1 byte
void IP_USART1_SendByte(uint8_t b);

// Gui nhieu byte lien tiep
void IP_USART1_SendBuf(const uint8_t *buf, uint32_t len);

// Cho den khi truyen xong hoan toan
void IP_USART1_WaitTC(void);

#endif
