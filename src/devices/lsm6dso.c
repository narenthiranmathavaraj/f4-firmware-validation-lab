#include "devices/lsm6dso.h"
#include "drivers/spi.h"

/* TODO (day 4): ST LSM6DSO datasheet: SPI read/write bit in the address byte, WHO_AM_I,
 * CTRL1_XL (ODR/FS), OUTX_L_A burst, sensitivity table. Mode 0 vs 3: check the datasheet. */
status_t lsm6dso_init(void) { return ERR_NOT_IMPL; }
status_t lsm6dso_read_accel_mg(int32_t xyz[3]) { (void)xyz; return ERR_NOT_IMPL; }
