#pragma once
#include <stdint.h>

void log_str(const char *s);
void log_hex(uint32_t v, int nibbles);          /* 0x.. fixed width */
void log_kv(const char *key, uint32_t value);   /* "key=0x........\n" */
void log_bytes(const char *tag, const uint8_t *p, int n); /* "tag: 01 a3 ..\n" */
void log_dec(int32_t v);