#pragma once
#include <stdint.h>
#include "common/status.h"

/* Register-level SPI master for STM32F4 SPI (Renode: STM32SPI). RM0090 "SPI" chapter.
 * CS is a GPIO you toggle yourself (or hardware NSS, your choice: document it). */
typedef struct
{
    uint8_t instance;          // 1 = SPI1, 2 = SPI2
    uint8_t cpol;              // 0 or 1
    uint8_t cpha;              // 0 or 1
    uint8_t baud_prescaler;    // BR encoding: 0 to 7

} spi_cfg_t;

status_t spi_init(const spi_cfg_t *cfg);
status_t spi_transfer(const spi_cfg_t *cfg, const uint8_t *tx, uint8_t *rx, uint32_t len);
