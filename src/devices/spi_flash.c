#include "devices/spi_flash.h"
#include "drivers/spi.h"

/* TODO (day 3): command set from the Micron MT25Q datasheet: RDID, WREN, RDSR (WIP bit),
 * READ, PP, SE. Always poll WIP with a timeout. Verify by readback. */
status_t flash_read_jedec(uint8_t id[3]) { (void)id; return ERR_NOT_IMPL; }
status_t flash_read(uint32_t a, uint8_t *b, uint32_t n) { (void)a; (void)b; (void)n; return ERR_NOT_IMPL; }
status_t flash_sector_erase(uint32_t a) { (void)a; return ERR_NOT_IMPL; }
status_t flash_page_program(uint32_t a, const uint8_t *b, uint32_t n) { (void)a; (void)b; (void)n; return ERR_NOT_IMPL; }
