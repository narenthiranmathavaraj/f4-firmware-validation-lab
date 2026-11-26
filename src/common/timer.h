/* common/timer.h */
#ifndef TIMER_H
#define TIMER_H

#include <stdint.h>

void timebase_init(void);
uint32_t timebase_us(void);
uint32_t timebase_ms(void);

#endif /* TIMER_H */
