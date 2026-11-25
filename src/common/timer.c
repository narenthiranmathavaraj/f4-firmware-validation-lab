/* timer.h */
#pragma once
#include <stdint.h>
void     timebase_init(void);
uint32_t timebase_us(void);   /* wraps every ~71 minutes: always compare using differences */
uint32_t timebase_ms(void);

/* timer.c */
#include "common/timer.h"
#include "stm32f4xx.h"

void timebase_init(void)
{
    RCC->APB1ENR |= RCC_APB1ENR_TIM2EN;
    TIM2->PSC = 15u;              /* 16 MHz / 16 = 1 MHz -> 1 us per count */
    TIM2->ARR = 0xFFFFFFFFu;      /* TIM2 is 32-bit on the F4 */
    TIM2->EGR = TIM_EGR_UG;
    TIM2->CR1 |= TIM_CR1_CEN;
}

uint32_t timebase_us(void) { return TIM2->CNT; }
uint32_t timebase_ms(void) { return TIM2->CNT / 1000u; }