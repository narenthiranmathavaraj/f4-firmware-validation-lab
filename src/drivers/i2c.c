#include "drivers/i2c.h"
#include "common/regs.h"
#include "common/log.h"

/* TODO (day 2): derive from the RM0090 I2C chapter, not from a HAL. F4 uses the older "v1" block:
 *  - registers: CR1, CR2 (FREQ), CCR, TRISE, SR1, SR2, DR
 *  - init: PE=0 -> CR2.FREQ, CCR, TRISE -> PE=1
 *  - write: START -> wait SB -> DR=addr<<1 -> wait ADDR (or AF=NACK) -> read SR1 then SR2 to clear
 *           -> wait TXE -> DR=data ... -> wait BTF -> STOP
 *  - read:  same start, addr<<1|1; 1-byte / 2-byte / N-byte reads have different ACK/STOP timing
 *           (the classic v1 pitfall: read the RM sequence carefully)
 *  - every wait bounded by a timeout; on AF set error and send STOP
 *  - log each transaction: "[i2c] addr=0x48 W 01 -> ACK"
 */


static void i2c_gpio_init(void)
{
    RCC_AHB1ENR |= (1u << 1);                         /* GPIOBEN */
    GPIOB_MODER   = (GPIOB_MODER   & ~((3u << 12) | (3u << 14))) | (2u << 12) | (2u << 14); /* PB6/PB7 = AF */
    GPIOB_OTYPER |= (1u << 6) | (1u << 7);            /* open-drain (I2C is wired-AND) */
    GPIOB_OSPEEDR = (GPIOB_OSPEEDR & ~((3u << 12) | (3u << 14))) | (2u << 12) | (2u << 14);
    GPIOB_PUPDR   = (GPIOB_PUPDR   & ~((3u << 12) | (3u << 14))) | (1u << 12) | (1u << 14); /* weak internal pull-up */
    GPIOB_AFRL    = (GPIOB_AFRL    & ~((0xFu << 24) | (0xFu << 28))) | (4u << 24) | (4u << 28); /* AF4 = I2C1 */
}

status_t i2c_init(void)
{
    RCC_APB1ENR |= (1u << 21);          /* I2C1EN */
    i2c_gpio_init();

    /* Reset to a known state (also clears a stuck BUSY flag) */
    I2C_CR1 &= ~CR1_PE;
    I2C_CR1 |= CR1_SWRST;
    I2C_CR1 &= ~CR1_SWRST;

    /* Timing: must be written while PE = 0 */
    I2C_CR2   = PCLK1_MHZ;                                /* FREQ = PCLK1 in MHz */
    I2C_CCR   = PCLK1_MHZ * 1000000u / (2u * I2C_SCL_HZ); /* std mode: Thigh = Tlow = CCR * Tpclk1 -> 80 */
    I2C_TRISE = PCLK1_MHZ + 1u;                           /* std mode: max rise 1000 ns -> FREQ + 1 */

    I2C_CR1 |= CR1_PE;                  /* enable */

    log_kv("i2c CR1", I2C_CR1);
    log_kv("i2c CR2", I2C_CR2);
    log_kv("i2c CCR", I2C_CCR);
    log_kv("i2c TRISE", I2C_TRISE);
    log_kv("i2c SR2", I2C_SR2);         /* BUSY (bit 1) should be 0 on an idle bus */

    return (I2C_CR1 & CR1_PE) ? ERR_OK : ERR_BUSY;
}

/* Wait until a flag in SR1 is set. Bounded. Returns ERR_NACK if AF shows up. */
static status_t wait_sr1(uint32_t mask)
{
    for (uint32_t t = I2C_TIMEOUT; t; t--) {
        uint32_t sr1 = I2C_SR1;
        if (sr1 & mask)   return ERR_OK;
        if (sr1 & SR1_AF) return ERR_NACK;
    }
    return ERR_TIMEOUT;
}

/* START (or repeated START) + address byte. Returns with ADDR set but NOT yet cleared. */
static status_t addr_phase(uint8_t a, uint32_t rw)
{
    status_t st;
    I2C_CR1 |= CR1_START;
    if ((st = wait_sr1(SR1_SB)) != ERR_OK)
        return st;
    I2C_DR = (uint32_t)(a << 1) | rw;          /* writing DR clears SB */
    return wait_sr1(SR1_ADDR);
}

static void clear_addr(void)
{
    volatile uint32_t tmp;
    tmp = I2C_SR1; tmp = I2C_SR2; (void)tmp;   /* read SR1 then SR2 clears ADDR */
}

/* Receive setup: must be done BEFORE the START that begins the read phase. */
static void rx_prepare(uint32_t n)
{
    I2C_CR1 |= CR1_ACK;
    if (n == 2) I2C_CR1 |= CR1_POS;
    else        I2C_CR1 &= ~CR1_POS;
}

static void rx_restore(void)
{
    I2C_CR1 &= ~CR1_POS;
    I2C_CR1 |= CR1_ACK;
}

/* Call with ADDR set. Clears it, receives n bytes (n >= 1), sends STOP. */
static status_t rx_phase(uint8_t *b, uint32_t n)
{
    status_t st;

    if (n == 1) {
        I2C_CR1 &= ~CR1_ACK;                   /* NACK the only byte: BEFORE clearing ADDR */
        clear_addr();
        I2C_CR1 |= CR1_STOP;
        if ((st = wait_sr1(SR1_RXNE)) != ERR_OK) return st;
        b[0] = (uint8_t)I2C_DR;
    } else if (n == 2) {
        clear_addr();
        I2C_CR1 &= ~CR1_ACK;                   /* with POS=1 this NACKs byte 2 */
        if ((st = wait_sr1(SR1_BTF)) != ERR_OK) return st;
        I2C_CR1 |= CR1_STOP;
        b[0] = (uint8_t)I2C_DR;
        b[1] = (uint8_t)I2C_DR;
    } else {
        clear_addr();
        for (uint32_t i = 0; i < n - 3; i++) {
            if ((st = wait_sr1(SR1_RXNE)) != ERR_OK) return st;
            b[i] = (uint8_t)I2C_DR;
        }
        if ((st = wait_sr1(SR1_BTF)) != ERR_OK) return st;
        I2C_CR1 &= ~CR1_ACK;
        b[n - 3] = (uint8_t)I2C_DR;
        I2C_CR1 |= CR1_STOP;
        b[n - 2] = (uint8_t)I2C_DR;
        if ((st = wait_sr1(SR1_RXNE)) != ERR_OK) return st;
        b[n - 1] = (uint8_t)I2C_DR;
    }
    return ERR_OK;
}

/* Send n bytes after ADDR is cleared; waits for BTF. No STOP. */
static status_t tx_phase(const uint8_t *b, uint32_t n)
{
    status_t st;
    for (uint32_t i = 0; i < n; i++) {
        if ((st = wait_sr1(SR1_TXE)) != ERR_OK) return st;
        I2C_DR = b[i];
    }
    return wait_sr1(SR1_BTF);
}

/* Common error exit: release the bus, clear AF, restore defaults. */
static status_t bus_fail(status_t st)
{
    I2C_CR1 |= CR1_STOP;
    rx_restore();
    I2C_SR1 &= ~SR1_AF;
    return st;
}

status_t i2c_write(uint8_t a, const uint8_t *b, uint32_t n)
{
    status_t st = addr_phase(a, 0);
    if (st != ERR_OK) return bus_fail(st);
    clear_addr();
    if ((st = tx_phase(b, n)) != ERR_OK) return bus_fail(st);
    I2C_CR1 |= CR1_STOP;
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