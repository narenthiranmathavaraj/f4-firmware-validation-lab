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
status_t i2c_init(void) { (void)I2C1_BASE; return ERR_NOT_IMPL; }
status_t i2c_write(uint8_t a, const uint8_t *b, uint32_t n) { (void)a; (void)b; (void)n; return ERR_NOT_IMPL; }
status_t i2c_read(uint8_t a, uint8_t *b, uint32_t n) { (void)a; (void)b; (void)n; return ERR_NOT_IMPL; }
status_t i2c_write_read(uint8_t a, const uint8_t *w, uint32_t wl, uint8_t *r, uint32_t rl)
{ (void)a; (void)w; (void)wl; (void)r; (void)rl; return ERR_NOT_IMPL; }
