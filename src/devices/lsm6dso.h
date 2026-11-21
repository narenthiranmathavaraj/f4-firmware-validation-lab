#pragma once
#include <stdint.h>
#include "common/status.h"

status_t lsm6dso_init(void);                       /* WHO_AM_I check, ODR, full-scale */
status_t lsm6dso_read_accel_mg(int32_t xyz_mg[3]); /* burst read + sensitivity scaling */
