#include "drivers/uart.h"
#include "common/log.h"

int main(void)
{
    uart_init();
    log_str("f4-bringup boot\n");
    log_kv("build", 1);
    log_str("BOOT_OK\n");

    /* Milestones: uncomment as you implement them. Each prints its raw bytes + decoded values.
     *   day 2: tmp108_init(); tmp108_read_mC(...)
     *   day 3: flash_read_jedec(...); erase/program/verify
     *   day 4: lsm6dso_init(); fault-injection selftests
     */
    for (;;) __asm volatile("wfi");
}
