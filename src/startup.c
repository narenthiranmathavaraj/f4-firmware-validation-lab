#include <stdint.h>

extern uint32_t _estack, _sidata, _sdata, _edata, _sbss, _ebss;
extern int main(void);

void Reset_Handler(void);
void Default_Handler(void) { for (;;) { } }

/* Core exceptions. Weak so you can override them by name (e.g. SysTick_Handler). */
#define WEAK_ALIAS __attribute__((weak, alias("Default_Handler")))
void NMI_Handler(void)        WEAK_ALIAS;
void HardFault_Handler(void)  WEAK_ALIAS;
void MemManage_Handler(void)  WEAK_ALIAS;
void BusFault_Handler(void)   WEAK_ALIAS;
void UsageFault_Handler(void) WEAK_ALIAS;
void SVC_Handler(void)        WEAK_ALIAS;
void PendSV_Handler(void)     WEAK_ALIAS;
void SysTick_Handler(void)    WEAK_ALIAS;

/* 16 core entries + 91 device IRQs (covers F405..F429). Fill device IRQs by index when you need them
 * (I2C1_EV, SPI1, ...): see the NVIC vector table in RM0090. */
#define NUM_IRQ 91
__attribute__((section(".isr_vector"), used))
void (*const vector_table[16 + NUM_IRQ])(void) = {
    (void (*)(void))&_estack,
    Reset_Handler, NMI_Handler, HardFault_Handler, MemManage_Handler,
    BusFault_Handler, UsageFault_Handler, 0, 0, 0, 0,
    SVC_Handler, 0, 0, PendSV_Handler, SysTick_Handler,
    [16 ... 16 + NUM_IRQ - 1] = Default_Handler,
};

void Reset_Handler(void)
{
    uint32_t *src = &_sidata, *dst = &_sdata;
    while (dst < &_edata) *dst++ = *src++;
    for (dst = &_sbss; dst < &_ebss; ) *dst++ = 0;
    (void)main();
    for (;;) { }
}
