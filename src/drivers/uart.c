#include "drivers/uart.h"
#include "common/status.h"
#include "common/log.h"
#include "stm32f4xx.h"

/* USART2. Renode does not model clocks/baud; real HW also needs GPIO AF7 on PA2/PA3 + BRR. */
void uart_init(void)
{
    RCC->APB1ENR |= RCC_APB1ENR_USART2EN;
    USART2->CR1 = USART_CR1_UE | USART_CR1_TE;
}

void uart_putc(char c)
{
    while (!(USART2->SR & USART_SR_TXE)) { }
    USART2->DR = (uint32_t)c;
}