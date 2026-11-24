#pragma once
#include <stdint.h>
#include "common/status.h"
#include "drivers/spi.h"


#define WHO_AM_I_REG     0x0F
#define OUTX_L_A     0x28
#define OUTY_L_A     0x29
#define OUTz_L_A     0x30
#define LSM6DSO_ID        0x6C



status_t lsm6dso_init(const spi_cfg_t *cfg);                         /* WHO_AM_I check */
status_t lsm6dso_read_accel_mg(const spi_cfg_t *cfg, int32_t xyz[3]); /* burst read */
