#pragma once
void uart_dma_init(void);
void uart_dma_wait(void);
void uart_dma_transmit(uint8_t *data, uint16_t length);
