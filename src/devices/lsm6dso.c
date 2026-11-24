#include "devices/lsm6dso.h"
#include "drivers/spi.h"

/* TODO (day 4): ST LSM6DSO datasheet: SPI read/write bit in the address byte, WHO_AM_I,
 CTRL1_XL (ODR/FS), OUTX_L_A burst, sensitivity table. Mode 0 vs 3: check the datasheet. */

status_t lsm6dso_init(const spi_cfg_t *cfg)
{
    uint8_t tx[2] = {0};
    uint8_t rx[2] = {0};

    // WHO_AM_I read
    tx[0] = 0x80 | WHO_AM_I_REG;
    tx[1] = 0x00;



    status_t st = spi_transfer(cfg, tx, rx, 2);
    if (st != ERR_OK) return st;

    return (rx[1] == LSM6DSO_ID) ? ERR_OK : ERR_BADID;
}

status_t lsm6dso_read_accel_mg(const spi_cfg_t *cfg, int32_t xyz[3]) {

    uint8_t tx[7] = {0};
    uint8_t rx[7] = {0};
    // Accelerometer burst read
    tx[0] = 0x80 | 0x28;
    status_t st = spi_transfer(cfg, tx, rx, 7);
    if (st != ERR_OK)
        return st;

    xyz[0] = (int16_t)((rx[2] << 8) | rx[1]);
    xyz[1] = (int16_t)((rx[4] << 8) | rx[3]);
    xyz[2] = (int16_t)((rx[6] << 8) | rx[5]);
   return ERR_OK;

}
