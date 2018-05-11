#ifndef PWM_H
#define PWM_H

#include <libopencm3/stm32/rcc.h>
#include <libopencm3/stm32/gpio.h>
#include <libopencm3/stm32/timer.h>
#include <libopencm3/stm32/exti.h>
#include <libopencm3/cm3/nvic.h>

#define PWM_PORT GPIOA
#define PWM_PIN GPIO4
#define PWM_RCC RCC_TIM14
#define PWM_AF GPIO_AF4
#define PWM_TIMER TIM14
#define PWM_CHAN TIM_OC1

/* Timer clock frequency [Hz] */
#define TIM_CLOCK_FREQ_HZ					1000000	/* 1MHz */

/* Default PWM frequency */
#define TIM_DEFAULT_PWM_FREQ_HZ				100	/* 100Hz */


#define LED_MODE_OFF 0
#define LED_MODE_SOFT_BLINK 1
#define LED_MODE_ON 2
#define LED_MODE_HARD_BLINK 3

#define PWM_INCR 5

void led_timer_setup(void);
void led_pwm_init(enum rcc_periph_clken rcc, uint32_t timer);
void led_pwm_setup(uint32_t timer, uint32_t chan, uint32_t port, uint8_t pin, uint8_t af, uint32_t mode);
void led_pwm_reset(uint32_t timer);
void led_pwm_start(uint32_t timer);
void led_pwm_set_dc(uint32_t timer, uint32_t chan, uint16_t dc_value_permillage);

#endif