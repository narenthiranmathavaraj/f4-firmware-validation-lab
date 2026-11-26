#include "drivers/i2c.h"
#include "common/status.h"
#include "common/log.h"
#include "stm32f4xx.h"

/* STM32F4 I2C "v1" block, RM0090 I2C chapter. I2C1 on PB6 (SCL) / PB7 (SDA), AF4.
 * Register and bit names come from the CMSIS device header (stm32f4xx.h). */

#define I2C_TIMEOUT 5000u

/* Clock assumption: HSI 16 MHz, APB1 prescaler 1 -> PCLK1 = 16 MHz (reset default) */
#define PCLK1_MHZ   16u
#define I2C_SCL_HZ  100000u   /* standard mode */

static void i2c_gpio_init(void)
{
    RCC->AHB1ENR |= RCC_AHB1ENR_GPIOBEN;
    GPIOB->MODER   = (GPIOB->MODER   & ~(GPIO_MODER_MODER6   | GPIO_MODER_MODER7))
                   | GPIO_MODER_MODER6_1 | GPIO_MODER_MODER7_1;            /* alternate function Mode Register */
    GPIOB->OTYPER |= GPIO_OTYPER_OT6 | GPIO_OTYPER_OT7;                    /* open-drain Output Type Register */
    GPIOB->OSPEEDR = (GPIOB->OSPEEDR & ~(GPIO_OSPEEDR_OSPEED6 | GPIO_OSPEEDR_OSPEED7))
                   | GPIO_OSPEEDR_OSPEED6_1 | GPIO_OSPEEDR_OSPEED7_1;      /* fast Output Speed Register */
    GPIOB->PUPDR   = (GPIOB->PUPDR   & ~(GPIO_PUPDR_PUPD6    | GPIO_PUPDR_PUPD7))
                   | GPIO_PUPDR_PUPD6_0 | GPIO_PUPDR_PUPD7_0;              /* weak pull-up */
    GPIOB->AFR[0]  = (GPIOB->AFR[0]  & ~(GPIO_AFRL_AFSEL6    | GPIO_AFRL_AFSEL7))
                   | GPIO_AFRL_AFSEL6_2 | GPIO_AFRL_AFSEL7_2;              /* AF4 = I2C1 */
}

status_t i2c_init(void)
{
    RCC->APB1ENR |= RCC_APB1ENR_I2C1EN;
    i2c_gpio_init();

    /* Reset to a known state (also clears a stuck BUSY flag) */
    I2C1->CR1 &= ~I2C_CR1_PE;
    I2C1->CR1 |= I2C_CR1_SWRST;
    I2C1->CR1 &= ~I2C_CR1_SWRST;
    I2C1->CR1 &= ~I2C_CR1_POS;
    /* Timing: must be written while PE = 0 */
    I2C1->CR2   = PCLK1_MHZ;                                 /* FREQ = PCLK1 in MHz */
    I2C1->CCR   = PCLK1_MHZ * 1000000u / (2u * I2C_SCL_HZ);  /* std mode: Thigh = Tlow = CCR * Tpclk1 -> 80 */
    I2C1->TRISE = PCLK1_MHZ + 1u;                            /* std mode: max rise 1000 ns -> FREQ + 1 */

    I2C1->CR1 |= I2C_CR1_PE;

    log_kv("i2c CR1", I2C1->CR1);
    log_kv("i2c CR2", I2C1->CR2);
    log_kv("i2c CCR", I2C1->CCR);
    log_kv("i2c TRISE", I2C1->TRISE);
    log_kv("i2c SR2", I2C1->SR2);        /* BUSY (bit 1) should be 0 on an idle bus */

    return (I2C1->CR1 & I2C_CR1_PE) ? ERR_OK : ERR_BUSY;
}

/* ---- helpers ------------------------------------------------------------------------ */

/* Wait until a flag in SR1 is set. Bounded. Returns ERR_NACK if AF (ack failure) shows up. */
static status_t wait_sr1(uint32_t mask)
{
    for (uint32_t t = I2C_TIMEOUT; t; t--) {
        uint32_t sr1 = I2C1->SR1;
        if (sr1 & mask)       return ERR_OK;
        if (sr1 & I2C_SR1_AF) return ERR_NACK;
    }
    return ERR_TIMEOUT;
}

/* START (or repeated START) + address byte. Returns with ADDR set but NOT yet cleared. */
static status_t addr_phase(uint8_t a, uint32_t rw)
{
    status_t st;
    I2C1->CR1 |= I2C_CR1_START;
    if ((st = wait_sr1(I2C_SR1_SB)) != ERR_OK) return st;
    I2C1->DR = (uint32_t)(a << 1) | rw;          /* writing DR clears SB */
    return wait_sr1(I2C_SR1_ADDR);
}

static void clear_addr(void)
{
    volatile uint32_t tmp;
    tmp = I2C1->SR1; tmp = I2C1->SR2; (void)tmp; /* read SR1 then SR2 clears ADDR */
}

/* Receive setup: must be done BEFORE the START that begins the read phase. */
static void rx_prepare(uint32_t n)
{
    I2C1->CR1 |= I2C_CR1_ACK;
    if (n == 2) I2C1->CR1 |= I2C_CR1_POS;        /* 2-byte case: ACK/NACK applies to the NEXT byte */
    else        I2C1->CR1 &= ~I2C_CR1_POS;
}

static void rx_restore(void)
{
    I2C1->CR1 &= ~I2C_CR1_POS;
    I2C1->CR1 |= I2C_CR1_ACK;
}

/* Call with ADDR set. Clears it, receives n bytes (n >= 1), sends STOP. */
static status_t rx_phase(uint8_t *b, uint32_t n)
{
    status_t st;

    if (n == 1) {
        I2C1->CR1 &= ~I2C_CR1_ACK;               /* NACK the only byte: BEFORE clearing ADDR */
        clear_addr();
        I2C1->CR1 |= I2C_CR1_STOP;
        if ((st = wait_sr1(I2C_SR1_RXNE)) != ERR_OK) return st;
        b[0] = (uint8_t)I2C1->DR;
    }
    else if (n == 2) {
        clear_addr();
        if ((st = wait_sr1(I2C_SR1_BTF)) != ERR_OK) return st;

        I2C1->CR1 &= ~I2C_CR1_ACK;               /* with POS=1 this NACKs byte 2 */
        I2C1->CR1 |= I2C_CR1_STOP;
        b[0] = (uint8_t)I2C1->DR;
        b[1] = (uint8_t)I2C1->DR;
    }
    else {
        clear_addr();
        for (uint32_t i = 0; i < n - 3; i++) {
            if ((st = wait_sr1(I2C_SR1_RXNE)) != ERR_OK) return st;
            b[i] = (uint8_t)I2C1->DR;
        }
        if ((st = wait_sr1(I2C_SR1_BTF)) != ERR_OK) return st;
        I2C1->CR1 &= ~I2C_CR1_ACK;
        b[n - 3] = (uint8_t)I2C1->DR;
        I2C1->CR1 |= I2C_CR1_STOP;
        b[n - 2] = (uint8_t)I2C1->DR;
        if ((st = wait_sr1(I2C_SR1_RXNE)) != ERR_OK) return st;
        b[n - 1] = (uint8_t)I2C1->DR;
    }
    return ERR_OK;
}

/* Send n bytes after ADDR is cleared; waits for BTF. No STOP. */
static status_t tx_phase(const uint8_t *b, uint32_t n)
{
    status_t st;
    for (uint32_t i = 0; i < n; i++) {
        if ((st = wait_sr1(I2C_SR1_TXE)) != ERR_OK) return st;
        I2C1->DR = b[i];
    }
    return wait_sr1(I2C_SR1_BTF);
}

/* Common error exit: release the bus, clear AF, restore defaults. */
static status_t bus_fail(status_t st)
{
    I2C1->CR1 |= I2C_CR1_STOP;
    rx_restore();
    I2C1->SR1 &= ~I2C_SR1_AF;
    return st;
}

/* ---- public API --------------------------------------------------------------------- */

status_t i2c_write(uint8_t a, const uint8_t *b, uint32_t n)
{
    status_t st = addr_phase(a, 0);
    if (st != ERR_OK) return bus_fail(st);
    clear_addr();
    if ((st = tx_phase(b, n)) != ERR_OK) return bus_fail(st);
    I2C1->CR1 |= I2C_CR1_STOP;
    return ERR_OK;
}

status_t i2c_read(uint8_t a, uint8_t *b, uint32_t n)
{
    status_t st;
    if (n == 0) return ERR_OK;
    rx_prepare(n);
    st = addr_phase(a, 1);
    if (st == ERR_OK) st = rx_phase(b, n);
    if (st != ERR_OK) return bus_fail(st);
    rx_restore();
    return ERR_OK;
}

/* S | addr+W | A | w... | A | Sr | addr+R | A | r... | N | P */
status_t i2c_write_read(uint8_t a, const uint8_t *w, uint32_t wl, uint8_t *r, uint32_t rl)
{
    status_t st;
    if (wl == 0) return i2c_read(a, r, rl);
    if (rl == 0) return i2c_write(a, w, wl);

    st = addr_phase(a, 0);
    if (st != ERR_OK) return bus_fail(st);
    clear_addr();
    if ((st = tx_phase(w, wl)) != ERR_OK) return bus_fail(st);   /* NO STOP here */

    rx_prepare(rl);                              /* ACK/POS set before the repeated START */
    st = addr_phase(a, 1);                       /* repeated START + address, read */
    if (st == ERR_OK) st = rx_phase(r, rl);
    if (st != ERR_OK) return bus_fail(st);
    rx_restore();
    return ERR_OK;
}