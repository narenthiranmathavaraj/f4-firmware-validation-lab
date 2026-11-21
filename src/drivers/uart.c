#include "drivers/uart.h"
#include "common/regs.h"

/* USART (RM0090 USART chapter): SR @0x00, DR @0x04, BRR @0x08, CR1 @0x0C */
#define USART_SR   REG32(USART2_BASE + 0x00)
#define USART_DR   REG32(USART2_BASE + 0x04)
#define USART_CR1  REG32(USART2_BASE + 0x0C)

#define CR1_TE     (1u << 3)
#define CR1_UE     (1u << 13)
#define SR_TXE     (1u << 7)

void uart_init(void)
{
    /* Renode does not model clocks/baud; real HW also needs RCC clock + GPIO AF + BRR */
    USART_CR1 = CR1_UE | CR1_TE;
}

void uart_putc(char c)
{
    while (!(USART_SR & SR_TXE)) { }
    USART_DR = (uint32_t)c;
}
