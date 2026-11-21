#pragma once
#include <stdint.h>

#define REG32(addr)   (*(volatile uint32_t *)(addr))

/* Peripheral base addresses (RM0090 memory map; match platform/f4_bringup.repl) */
#define USART2_BASE   0x40004400u
#define I2C1_BASE     0x40005400u
#define SPI1_BASE     0x40013000u
#define SPI2_BASE     0x40003800u
