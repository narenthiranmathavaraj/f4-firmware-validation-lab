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

/* FS = +-2g: 0.061 mg/LSB = 125/2048 exactly, rounded to nearest */
static int32_t raw_to_mg(int16_t raw)
{
    int32_t n = (int32_t)raw * 125;
    return (n >= 0) ? (n + 1024) / 2048 : (n - 1024) / 2048;
}

status_t lsm6dso_read_accel_mg(const spi_cfg_t *cfg, int32_t xyz[3])
{
    uint8_t tx[7] = {0};
    uint8_t rx[7] = {0};

    tx[0] = 0x80 | 0x28;
    status_t st = spi_transfer(cfg, tx, rx, 7);
    if (st != ERR_OK)
        return st;

    int16_t raw[3];
    raw[0] = (int16_t)((rx[2] << 8) | rx[1]);
    raw[1] = (int16_t)((rx[4] << 8) | rx[3]);
    raw[2] = (int16_t)((rx[6] << 8) | rx[5]);

    for (int i = 0; i < 3; i++)
        xyz[i] = raw_to_mg(raw[i]);

    return ERR_OK;
}