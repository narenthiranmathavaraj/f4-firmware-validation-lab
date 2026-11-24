
#include "common/log.h"
#include "drivers/uart_dma.h"
#include <stddef.h>

#define LOG_BUFFER_SIZE 128

static char log_buffer[LOG_BUFFER_SIZE];
static uint16_t log_index;

/* Append one character to the message buffer. */
static void log_append_char(char c)
{
    if (log_index < LOG_BUFFER_SIZE - 1)
        log_buffer[log_index++] = c;
}

/* Append a complete string. */
static void log_append_str(const char *s)
{
    if (s == NULL)
        return;

    while (*s)
        log_append_char(*s++);
}

/* Append hexadecimal value. */
static void log_append_hex(uint32_t v, int nibbles)
{
    static const char d[] = "0123456789abcdef";

    if (nibbles < 1)
        return;

    if (nibbles > 8)
        nibbles = 8;

    log_append_str("0x");

    for (int i = nibbles - 1; i >= 0; i--)
        log_append_char(d[(v >> (4 * i)) & 0xF]);
}

/* Append signed decimal value. */
static void log_append_dec(int32_t v)
{
    char buf[10];
    int i = 0;

    uint32_t u = (v < 0)
        ? (uint32_t)(-(v + 1)) + 1U
        : (uint32_t)v;

    if (v < 0)
        log_append_char('-');

    if (u == 0)
    {
        log_append_char('0');
        return;
    }

    while (u)
    {
        buf[i++] = (char)('0' + (u % 10U));
        u /= 10U;
    }

    while (i)
        log_append_char(buf[--i]);
}

/* Transmit the complete message using DMA. */
static void log_flush(void)
{
    if (log_index == 0)
        return;

    uart_dma_transmit((uint8_t *)log_buffer, log_index);

    /*
     * Wait until DMA has transferred all bytes
     * and USART has transmitted the final byte.
     */
    uart_dma_wait();

    log_index = 0;
}

void log_str(const char *s)
{
    log_index = 0;
    log_append_str(s);
    log_flush();
}

void log_hex(uint32_t v, int nibbles)
{
    log_index = 0;
    log_append_hex(v, nibbles);
    log_flush();
}

void log_kv(const char *key, uint32_t value)
{
    log_index = 0;

    log_append_str(key);
    log_append_char('=');
    log_append_hex(value, 8);
    log_append_char('\n');

    log_flush();
}

void log_bytes(const char *tag, const uint8_t *p, int n)
{
    log_index = 0;

    log_append_str(tag);
    log_append_char(':');

    if (p != NULL && n > 0)
    {
        for (int i = 0; i < n; i++)
        {
            log_append_char(' ');
            log_append_hex(p[i], 2);
        }
    }

    log_append_char('\n');
    log_flush();
}

void log_dec(int32_t v)
{
    log_index = 0;
    log_append_dec(v);
    log_flush();
}


// #include "common/log.h"
// #include "drivers/uart.h"

// void log_str(const char *s) { while (*s) uart_putc(*s++); }

// void log_hex(uint32_t v, int nibbles)
// {
//     static const char d[] = "0123456789abcdef";
//     log_str("0x");
//     for (int i = nibbles - 1; i >= 0; i--) uart_putc(d[(v >> (4 * i)) & 0xF]);
// }

// void log_kv(const char *key, uint32_t value)
// {
//     log_str(key); log_str("="); log_hex(value, 8); log_str("\n");
// }

// void log_bytes(const char *tag, const uint8_t *p, int n)
// {
//     log_str(tag); log_str(":");
//     for (int i = 0; i < n; i++) { uart_putc(' '); log_hex(p[i], 2); }
//     log_str("\n");
// }
// void log_dec(int32_t v)
// {
//     char buf[11];
//     int i = 0;
//     uint32_t u = (v < 0) ? (uint32_t)(-(v + 1)) + 1u : (uint32_t)v;
//     if (v < 0) uart_putc('-');
//     if (u == 0) { uart_putc('0'); return; }
//     while (u) { buf[i++] = (char)('0' + (u % 10u)); u /= 10u; }
//     while (i) uart_putc(buf[--i]);
// }