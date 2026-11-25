#include "FreeRTOS.h"
#include "task.h"
#include "common/log.h"
#include "stm32f4xx.h"

/* Runs whenever no task is ready: sleep until the next interrupt */
void vApplicationIdleHook(void) { __WFI(); }

/* Called by the kernel when a task's stack canary is damaged */
void vApplicationStackOverflowHook(TaskHandle_t task, char *name)
{
    (void)task;
    __disable_irq();
    log_str("FATAL: stack overflow in task ");
    log_str(name);
    log_str("\n");
    NVIC_SystemReset();
}