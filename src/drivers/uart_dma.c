#include "drivers/uart.h"
#include "common/status.h"
#include "common/log.h"
#include "stm32f4xx.h"
#include "drivers/uart_dma.h"
/*#define USART_CR3_DMAT_Pos            (7U)
#define USART_CR3_DMAT_Msk            (0x1UL << USART_CR3_DMAT_Pos)*/
/* USART2. Renode does not model clocks/baud; real HW also needs GPIO AF7 on PA2/PA3 + BRR. */
void uart_dma_init(void)
{
    uart_init();
    RCC->AHB1ENR |= RCC_AHB1ENR_DMA1EN;
        // Disable DMA stream before configuring
    DMA1_Stream6->CR &= ~DMA_SxCR_EN;
    while (DMA1_Stream6->CR & DMA_SxCR_EN) {}
        // Clear DMA Stream 6 interrupt flags
    DMA1->HIFCR = DMA_HIFCR_CFEIF6 |
                  DMA_HIFCR_CDMEIF6 |
                  DMA_HIFCR_CTEIF6 |
                  DMA_HIFCR_CHTIF6 |
                  DMA_HIFCR_CTCIF6;

    // Peripheral address: USART2 data register
    DMA1_Stream6->PAR = (uint32_t)&USART2->DR;

    // byte-sized peripheral and memory transfers
    DMA1_Stream6->CR =
        (4U << DMA_SxCR_CHSEL_Pos) |
        DMA_SxCR_MINC |
        (1U << DMA_SxCR_DIR_Pos);

    // Enable USART2 DMA transmitter requests
    USART2->CR3 |= USART_CR3_DMAT;
}

void uart_dma_transmit(uint8_t *data, uint16_t length)
{
        // Check whether DMA is already transmitting
    if (DMA1_Stream6->CR & DMA_SxCR_EN)
        return;
    // Clear previous interrupt flags
    DMA1->HIFCR = DMA_HIFCR_CFEIF6 |
                  DMA_HIFCR_CDMEIF6 |
                  DMA_HIFCR_CTEIF6 |
                  DMA_HIFCR_CHTIF6 |
                  DMA_HIFCR_CTCIF6;

    // Set source buffer
    DMA1_Stream6->M0AR = (uint32_t)data;

    // Set transfer count
    DMA1_Stream6->NDTR = length;

    // Enable DMA stream
    DMA1_Stream6->CR |= DMA_SxCR_EN;
}
void uart_dma_wait(void)
{
    /* Wait until DMA has moved all bytes. */
    while (DMA1_Stream6->CR & DMA_SxCR_EN) {}

    /* Wait until the final UART stop bit is transmitted. */
    while (!(USART2->SR & USART_SR_TC)) {}
}