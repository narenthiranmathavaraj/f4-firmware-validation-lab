#pragma once
#include <stdint.h>
#include "common/status.h"

status_t flash_read_jedec(uint8_t id[3]);
status_t flash_read(uint32_t addr, uint8_t *buf, uint32_t len);
status_t flash_sector_erase(uint32_t addr);
status_t flash_page_program(uint32_t addr, const uint8_t *buf, uint32_t len);
