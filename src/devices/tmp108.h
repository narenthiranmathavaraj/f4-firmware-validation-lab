#pragma once
#include <stdint.h>
#include "common/status.h"

#define TMP108_ADDR 0x48u   /* from platform/f4_bringup.repl; datasheet: ADD0 pin strapping */

status_t tmp108_init(void);
status_t tmp108_read_raw(int16_t *raw);
status_t tmp108_read_mC(int32_t *milli_celsius);  /* raw -> milli-degC per datasheet */
status_t tmp108_shutdown(void);                   /* low-power mode */
