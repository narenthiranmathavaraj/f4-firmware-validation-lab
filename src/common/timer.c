
#include "timer.h"
#include "stm32f4xx.h"

#define CPU_CLOCK_HZ  16000000U
#define SYSTICK_HZ    1000U

static volatile uint32_t g_ms_ticks = 0;

void SysTick_Handler(void)
{
    g_ms_ticks++;
}

void timebase_init(void)
{
    SysTick_Config(CPU_CLOCK_HZ / SYSTICK_HZ);
}

uint32_t millis(void)
{
    return g_ms_ticks;
}