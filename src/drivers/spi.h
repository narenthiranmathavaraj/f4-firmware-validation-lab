#pragma once
#include <stdint.h>
#include "common/status.h"

/* Register-level SPI master for STM32F4 SPI (Renode: STM32SPI). RM0090 "SPI" chapter.
 * CS is a GPIO you toggle yourself (or hardware NSS, your choice: document it). */
typedef struct { uint32_t base; uint8_t cpol, cpha; uint32_t prescaler; } spi_cfg_t;

status_t spi_init(const spi_cfg_t *cfg);
status_t spi_transfer(const spi_cfg_t *cfg, const uint8_t *tx, uint8_t *rx, uint32_t len);
