#pragma once
#include <stdint.h>
#include "common/status.h"

/* Register-level I2C master for the STM32F4 I2C (v1) block (Renode: STM32F4_I2C).
 * RM0090 "I2C" chapter. Bounded waits only: every loop must time out. */
status_t i2c_init(void);                       /* TIMINGR, PE */
status_t i2c_write(uint8_t addr7, const uint8_t *buf, uint32_t len);
status_t i2c_read(uint8_t addr7, uint8_t *buf, uint32_t len);
status_t i2c_write_read(uint8_t addr7, const uint8_t *w, uint32_t wl, uint8_t *r, uint32_t rl);
