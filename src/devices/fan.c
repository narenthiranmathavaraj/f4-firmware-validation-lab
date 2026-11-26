#include "fan.h"
#include "stm32f4xx.h"

#define FAN_PWM_PRESCALER  83U
#define FAN_PWM_PERIOD     999U

status_t fan_init(void)
{
    /* Enable GPIOA clock */
    RCC->AHB1ENR |= RCC_AHB1ENR_GPIOAEN;

    /* Enable TIM2 clock */
    RCC->APB1ENR |= RCC_APB1ENR_TIM2EN;

    /* Configure PA0 as alternate function */
    GPIOA->MODER &= ~(3U << (0U * 2U));
    GPIOA->MODER |=  (2U << (0U * 2U));

    /* Select AF1 for TIM2_CH1 */
    GPIOA->AFR[0] &= ~(0xFU << (0U * 4U));
    GPIOA->AFR[0] |=  (1U   << (0U * 4U));

    /* Push-pull output */
    GPIOA->OTYPER &= ~(1U << 0U);

    /* High speed */
    GPIOA->OSPEEDR |= (3U << (0U * 2U));

    /* Stop timer during configuration */
    TIM2->CR1 &= ~TIM_CR1_CEN;

    /* Configure timer frequency */
    TIM2->PSC = FAN_PWM_PRESCALER;
    TIM2->ARR = FAN_PWM_PERIOD;

    /* Initial duty cycle: 0% */
    TIM2->CCR1 = 0U;

    /* PWM mode 1, channel 1 */
    TIM2->CCMR1 &= ~(TIM_CCMR1_CC1S |
                     TIM_CCMR1_OC1M);

    TIM2->CCMR1 |= TIM_CCMR1_OC1M_1 |
                   TIM_CCMR1_OC1M_2 |
                   TIM_CCMR1_OC1PE;

    /* Enable channel 1 output */
    TIM2->CCER |= TIM_CCER_CC1E;

    /* Enable auto-reload preload */
    TIM2->CR1 |= TIM_CR1_ARPE;

    /* Load configuration into timer registers */
    TIM2->EGR = TIM_EGR_UG;

    /* Start timer */
    TIM2->CR1 |= TIM_CR1_CEN;

    return ERR_OK;
}

void fan_set_duty(uint8_t duty)
{
    if (duty > 100U)
        duty = 100U;

    TIM2->CCR1 = ((FAN_PWM_PERIOD + 1U) * duty) / 100U;
}

void fan_on(void)
{
    fan_set_duty(100U);
}

void fan_off(void)
{
    fan_set_duty(0U);
}