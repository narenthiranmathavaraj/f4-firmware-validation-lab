#ifndef FAN_H
#define FAN_H

#include <stdint.h>
#include "common/status.h"

status_t fan_init(void);
void fan_set_duty(uint8_t duty);
void fan_on(void);
void fan_off(void);

#endif