#include "common/log.h"
#include "drivers/uart.h"

void log_str(const char *s) { while (*s) uart_putc(*s++); }

void log_hex(uint32_t v, int nibbles)
{
    static const char d[] = "0123456789abcdef";
    log_str("0x");
    for (int i = nibbles - 1; i >= 0; i--) uart_putc(d[(v >> (4 * i)) & 0xF]);
}

void log_kv(const char *key, uint32_t value)
{
    log_str(key); log_str("="); log_hex(value, 8); log_str("\n");
}

void log_bytes(const char *tag, const uint8_t *p, int n)
{
    log_str(tag); log_str(":");
    for (int i = 0; i < n; i++) { uart_putc(' '); log_hex(p[i], 2); }
    log_str("\n");
}
void log_dec(int32_t v)
{
    char buf[11];
    int i = 0;
    uint32_t u = (v < 0) ? (uint32_t)(-(v + 1)) + 1u : (uint32_t)v;
    if (v < 0) uart_putc('-');
    if (u == 0) { uart_putc('0'); return; }
    while (u) { buf[i++] = (char)('0' + (u % 10u)); u /= 10u; }
    while (i) uart_putc(buf[--i]);
}