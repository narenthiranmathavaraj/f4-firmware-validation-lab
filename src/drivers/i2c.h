#pragma once
#include <stdint.h>
#include "common/status.h"

/* Register-level I2C master for the STM32F4 I2C (v1) block (Renode: STM32F4_I2C).
 * RM0090 "I2C" chapter. Bounded waits only: every loop must time out. */
status_t i2c_init(void);                       /* TIMINGR, PE */
status_t i2c_write(uint8_t addr7, const uint8_t *buf, uint32_t len);
status_t i2c_read(uint8_t addr7, uint8_t *buf, uint32_t len);
status_t i2c_write_read(uint8_t addr7, const uint8_t *w, uint32_t wl, uint8_t *r, uint32_t rl);


#define I2C_CR1 REG32(I2C1_BASE + 0x00)
#define I2C_CR2 REG32(I2C1_BASE + 0x04)
#define I2C_OAR1 REG32(I2C1_BASE + 0x08) // needed if slave
#define I2C_OAR2 REG32(I2C1_BASE + 0x0C) // needed if slave
#define I2C_SR1 REG32(I2C1_BASE + 0x14)
#define I2C_SR2 REG32(I2C1_BASE + 0x18)
#define I2C_CCR REG32(I2C1_BASE + 0x1C)
#define I2C_TRISE REG32(I2C1_BASE + 0x20) //Rise Time Register
#define I2C_FLTR REG32(I2C1_BASE + 0x24)

#define I2C_DR REG32(I2C1_BASE + 0x10)
#define CR1_START  (1u << 8)
#define CR1_ACK (1u << 10)
#define CR1_ACK    (1u << 10)
#define CR1_POS    (1u << 11)
#define SR1_RXNE   (1u << 6)
#define CR1_STOP   (1u << 9)
#define SR1_SB     (1u << 0)
#define SR1_ADDR   (1u << 1)
#define SR1_BTF    (1u << 2)
#define SR1_TXE    (1u << 7)
#define SR1_AF     (1u << 10)
#define I2C_TIMEOUT 100000u

#define CR1_PE     (1u << 0)
#define CR1_SWRST  (1u << 15)

#define PCLK1_MHZ   16u
#define I2C_SCL_HZ  100000u   // standard mode

/* RCC + GPIOB (needed on real hardware; Renode accepts the writes) */
#define RCC_AHB1ENR   REG32(0x40023800u + 0x30) //RCC Reset Clock Control Base address + offset  //I2C1 clock
#define RCC_APB1ENR   REG32(0x40023800u + 0x40) // GPIOB clock
#define GPIOB_MODER   REG32(0x40020400u + 0x00) //GPIO B base address MODER: alternate function, which hands the pin to a peripheral
#define GPIOB_OTYPER  REG32(0x40020400u + 0x04) //OTYPER: open-drain, because I2C lines are wired-AND and devices only pull low
#define GPIOB_OSPEEDR REG32(0x40020400u + 0x08) // OSPEEDR: output speed
#define GPIOB_PUPDR   REG32(0x40020400u + 0x0C) // PUPDR: pull-up, so the line is high when idle
#define GPIOB_AFRL    REG32(0x40020400u + 0x20) // AFRL: which peripheral, here AF4 = I2C1
