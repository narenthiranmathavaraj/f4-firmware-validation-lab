#pragma once
#include <stdint.h>
#include "common/status.h"

#define TMP108_ADDR 0x48u   /* from platform/f4_bringup.repl; datasheet: ADD0 pin strapping */
#define TMP108_REG_CONFIG 0x01
#define TMP108_REG_TEMP 0x00
#define TMP108_CR_DEFAULT_MSB 0x22
#define TMP108_CR_DEFAULT_LSB 0x10
#define TMP108_CR_M1 (1u<<1)
#define TMP108_CR_M0 (1u<<0)


status_t tmp108_init(void);
status_t tmp108_read_raw(int16_t *raw);
status_t tmp108_read_mC(int32_t *milli_celsius);  /* raw -> milli-degC per datasheet */
status_t tmp108_shutdown(void);                   /* low-power mode */
