#include "drivers/uart.h"
#include "drivers/i2c.h"
#include "devices/tmp108.h"
#include "common/log.h"

int main(void)
{
    uart_init();
    log_str("f4-bringup boot\n");
    log_str("BOOT_OK\n");

    log_kv("i2c_init status", (uint32_t)i2c_init());
    log_kv("tmp108_init status", (uint32_t)tmp108_init());

    int16_t raw = 0;
    int32_t mC = 0;
    log_kv("tmp108 read_raw status", (uint32_t)tmp108_read_raw(&raw));
    log_kv("tmp108 raw", (uint16_t)raw);
    log_kv("tmp108 read_mC status", (uint32_t)tmp108_read_mC(&mC));
    log_str("tmp108 mC=");
    log_dec(mC);
    log_str("\n");

    for (;;) __asm volatile("wfi");
}