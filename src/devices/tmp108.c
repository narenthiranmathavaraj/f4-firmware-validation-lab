#include "devices/tmp108.h"
#include "drivers/i2c.h"

/* TODO (day 2): TI TMP108 datasheet: pointer register, temperature register format,
 * configuration register (conversion mode, rate), shutdown / one-shot. Cite pages in
 * docs/datasheet_to_code.md. */
status_t tmp108_init(void) { return ERR_NOT_IMPL; }
status_t tmp108_read_raw(int16_t *raw) { (void)raw; return ERR_NOT_IMPL; }
status_t tmp108_read_mC(int32_t *m) { (void)m; return ERR_NOT_IMPL; }
status_t tmp108_shutdown(void) { return ERR_NOT_IMPL; }
