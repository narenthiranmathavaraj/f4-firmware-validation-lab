#include <stddef.h>
#include <stdint.h>

/* Minimal freestanding mem* (we link with -nostdlib). The attribute stops GCC from turning
 * these loops back into calls to themselves. */
#define NO_LOOP_IDIOM __attribute__((optimize("no-tree-loop-distribute-patterns")))

NO_LOOP_IDIOM void *memset(void *d, int c, size_t n)
{
    unsigned char *p = d;
    while (n--) *p++ = (unsigned char)c;
    return d;
}

NO_LOOP_IDIOM void *memcpy(void *d, const void *s, size_t n)
{
    unsigned char *dp = d; const unsigned char *sp = s;
    while (n--) *dp++ = *sp++;
    return d;
}

NO_LOOP_IDIOM void *memmove(void *d, const void *s, size_t n)
{
    unsigned char *dp = d; const unsigned char *sp = s;
    if (dp < sp) { while (n--) *dp++ = *sp++; }
    else         { dp += n; sp += n; while (n--) *--dp = *--sp; }
    return d;
}

NO_LOOP_IDIOM int memcmp(const void *a, const void *b, size_t n)
{
    const unsigned char *x = a, *y = b;
    while (n--) { if (*x != *y) return (int)*x - (int)*y; x++; y++; }
    return 0;
}