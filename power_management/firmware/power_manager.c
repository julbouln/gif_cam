/*
 * Based on stm32l-discovery button-irq-printf-lowpower from libopencm3-example
 *
 * This library is free software: you can redistribute it and/or modify
 * it under the terms of the GNU Lesser General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public License
 * along with this library.  If not, see <http://www.gnu.org/licenses/>.
 */

#include <libopencm3/stm32/rcc.h>
#include <libopencm3/stm32/gpio.h>
#include <libopencm3/stm32/rtc.h>
#include <libopencm3/stm32/pwr.h>
#include <libopencm3/stm32/exti.h>
#include <libopencm3/stm32/flash.h>

#include <libopencm3/stm32/timer.h>

#include <libopencm3/cm3/nvic.h>
#include <libopencm3/cm3/scb.h>

#include <libopencm3/stm32/adc.h>

#include "rtc.h"
#include "pwm.h"

// power pin must be 5V tolerant
#define POWER_PORT GPIOA
#define POWER_PIN GPIO14

#define SHUTDOWN_PORT GPIOA
#define SHUTDOWN_PIN GPIO13

#define ALIVE_PORT GPIOA
#define ALIVE_PIN GPIO5

#define PWM_PORT_R GPIOB
#define PWM_PIN_R GPIO1
#define PWM_RCC_R RCC_TIM14
#define PWM_AF_R GPIO_AF0
#define PWM_TIMER_R TIM14
#define PWM_CHAN_R TIM_OC1

#define PWM_PORT_G GPIOA
#define PWM_PIN_G GPIO7
#define PWM_RCC_G RCC_TIM3
#define PWM_AF_G GPIO_AF1
#define PWM_TIMER_G TIM3
#define PWM_CHAN_G TIM_OC2

#define PWM_PORT_B GPIOA
#define PWM_PIN_B GPIO6
#define PWM_RCC_B RCC_TIM3
#define PWM_AF_B GPIO_AF1
#define PWM_TIMER_B TIM3
#define PWM_CHAN_B TIM_OC1

#define BAT_SENSE_PORT GPIOA
#define BAT_SENSE_PIN GPIO1

#define BAT_MON_PORT GPIOA
#define BAT_MON_PIN GPIO4

void scb_enable_deep_sleep_mode(void);
void gpio_setup(void);
void gpio_irq_setup(void);
void wait(int ms);
void power_down_delay(int min, int sec);
void power_down_postpone(void);
void rtc_setup(void);
void rcc_clock_setup_in_hsi_out_8mhz(void);
void clock_setup(void);
void standby(void);
void adc_setup(void);
float bat_sense(void);
void led_set_mode(uint8_t mode);
void led_set_luminance(float l);
void led_set_color(uint8_t r, uint8_t g, uint8_t b);
void power_up(void);
void power_down(void);

static inline __attribute__((always_inline)) void __WFI(void)
{
	__asm volatile ("wfi");
}

static inline __attribute__((always_inline)) void __NOP(void)
{
	__asm volatile ("nop");
}

void scb_enable_deep_sleep_mode(void)
{
	SCB_SCR |= SCB_SCR_SLEEPDEEP;
}

void gpio_setup(void)
{
	/* WAKE UP pin */
	gpio_mode_setup(GPIOA, GPIO_MODE_INPUT, GPIO_PUPD_PULLDOWN, GPIO0);

	/* POWER pin */
	gpio_mode_setup(POWER_PORT, GPIO_MODE_OUTPUT, GPIO_PUPD_NONE, POWER_PIN);

	/* SHUTDOWN pin */
	gpio_mode_setup(SHUTDOWN_PORT, GPIO_MODE_OUTPUT, GPIO_PUPD_NONE, SHUTDOWN_PIN);

	/* BATTERY SENSE pin */
	gpio_mode_setup(BAT_SENSE_PORT, GPIO_MODE_ANALOG, GPIO_PUPD_NONE, BAT_SENSE_PIN);

	/* BATTERY MONITOR pin */
	gpio_mode_setup(BAT_MON_PORT, GPIO_MODE_OUTPUT, GPIO_PUPD_PULLDOWN, BAT_MON_PIN);

	/* ALIVE pin */
	gpio_mode_setup(ALIVE_PORT, GPIO_MODE_INPUT, GPIO_PUPD_PULLDOWN, ALIVE_PIN);

}

// WAKE UP pin and ALIVE pin should postpone shut down via IRQ
void gpio_irq_setup() {
	// EXTI0_1 for WAKE UP interrupt
	exti_select_source(EXTI0, GPIOA);
	exti_set_trigger(EXTI0, EXTI_TRIGGER_RISING);
	exti_enable_request(EXTI0);
	nvic_enable_irq(NVIC_EXTI0_1_IRQ);

	// EXTI4_15 for ALIVE interrupt
	exti_select_source(EXTI5, GPIOA);
	exti_set_trigger(EXTI5, EXTI_TRIGGER_RISING);
	exti_enable_request(EXTI5);
	nvic_enable_irq(NVIC_EXTI4_15_IRQ);
}

#define FREQUENCY 48000000

void wait(int ms) {
	int w = FREQUENCY / 10000 * ms;
	int l;
	for (l = 0; l < w; l++) {
		__NOP();
	}
}

/* set next alarm: current time + min:sec */
void power_down_delay(int min, int sec)
{
	struct rtc_alarm alarm = {0};
	struct rtc_time time = {0};
	int val;

	/* read current min/sec values */
	rtc_read_calendar(&time, 0);

	/* BCD min/sec math: calculate next alarm */
	val = time.su + sec;
	alarm.su = val % 10;

	val = time.st + val / 10;
	alarm.st = val % 6;

	val = time.mnu + min + val / 6;
	alarm.mnu = val % 10;

	val = time.mnt + val / 10;
	alarm.mnt = val % 6;

	/* this app cares about min/sec only */
	alarm.msk1 = 0;
	alarm.msk2 = 0;
	alarm.msk3 = 1;
	alarm.msk4 = 1;

	/* set new alarm */
	rtc_set_alarm(&alarm);
}

void power_down_postpone() {
	// delay power down - 15 minutes
	power_down_delay(15, 0);
}

void rtc_setup(void)
{
	/* reset RTC */
	rcc_periph_reset_pulse(RST_BACKUPDOMAIN);

	/* enable LSI clock */
	rcc_osc_on(RCC_LSI);
	rcc_wait_for_osc_ready(RCC_LSI);

	/* select LSI clock for RTC */
	rtc_unlock();
	rcc_set_rtc_clock_source(RCC_LSI);
	rcc_enable_rtc_clock();
	rtc_lock();
}

void rcc_clock_setup_in_hsi_out_8mhz(void)
{
	rcc_osc_on(RCC_HSI);
	rcc_wait_for_osc_ready(RCC_HSI);
	rcc_set_sysclk_source(RCC_HSI);

	rcc_set_hpre(RCC_CFGR_HPRE_NODIV);
	rcc_set_ppre(RCC_CFGR_PPRE_NODIV);

	flash_prefetch_buffer_enable();
	flash_set_ws(FLASH_ACR_LATENCY_024_048MHZ);

	/* 8MHz * 2 / 2 = 8MHz */
	rcc_set_pll_multiplication_factor(RCC_CFGR_PLLMUL_MUL2);
	rcc_set_pll_source(RCC_CFGR_PLLSRC_HSI_CLK_DIV2);

	rcc_osc_on(RCC_PLL);
	rcc_wait_for_osc_ready(RCC_PLL);
	rcc_set_sysclk_source(RCC_PLL);

	rcc_apb1_frequency = 8000000;
	rcc_ahb_frequency = 8000000;
}

void clock_setup(void)
{
	rcc_clock_setup_in_hsi_out_48mhz();

	rcc_periph_clock_enable(RCC_GPIOA);
	rcc_periph_clock_enable(RCC_GPIOB);
	rcc_periph_clock_enable(RCC_RTC);
	rcc_periph_clock_enable(RCC_PWR);
	rcc_periph_clock_enable(RCC_ADC);

	/* enable rtc unlocking */
	pwr_disable_backup_domain_write_protect();
}

void standby() {
	scb_enable_deep_sleep_mode();
	pwr_clear_wakeup_flag();
	pwr_clear_standby_flag();
	pwr_set_standby_mode();
	pwr_disable_power_voltage_detect();
	pwr_enable_wakeup_pin();
	__WFI();
}

#if 1

/* BAT MONITOR */

uint8_t channel_array[] = { 1, 1, ADC_CHANNEL_TEMP};

void adc_setup(void)
{
	adc_power_off(ADC1);
	adc_set_clk_source(ADC1, ADC_CLKSOURCE_ADC);
	adc_calibrate(ADC1);
	adc_set_operation_mode(ADC1, ADC_MODE_SCAN);
	adc_disable_external_trigger_regular(ADC1);
	adc_set_right_aligned(ADC1);
	adc_enable_temperature_sensor();
	adc_set_sample_time_on_all_channels(ADC1, ADC_SMPTIME_071DOT5);
	adc_set_regular_sequence(ADC1, 1, channel_array);
	adc_set_resolution(ADC1, ADC_RESOLUTION_12BIT);
	adc_disable_analog_watchdog(ADC1);
	adc_power_on(ADC1);

	/* Wait for ADC starting up. */
	wait(200);
}

float bat_sense() {
	uint16_t bat;

	gpio_set(BAT_MON_PORT, BAT_MON_PIN);
	adc_start_conversion_regular(ADC1);
	while (!(adc_eoc(ADC1)));

	bat = adc_read_regular(ADC1);
	gpio_clear(BAT_MON_PORT, BAT_MON_PIN);

	return (float)bat / 4095.0 * 3.2;
}

/* PWM */

uint8_t cur_led_mode = 0;
uint16_t cur_led_oc = 0;
int cur_led_dir = 1;

float led_lum = 1.0;
uint8_t led_r = 255;
uint8_t led_g = 255;
uint8_t led_b = 255;

void led_set_mode(uint8_t mode) {
	cur_led_mode = mode;
}

void led_set_luminance(float l) {
	led_lum = l;
}

void led_set_color(uint8_t r, uint8_t g, uint8_t b) {
	led_r = r;
	led_g = g;
	led_b = b;
}

#define LED_MAX_OC 1000

/* POWER */

int power_down_event;
bool started;
float cur_bat_v;

void power_up() {
	// enable power board
	gpio_set(POWER_PORT, POWER_PIN);
	gpio_set(SHUTDOWN_PORT, SHUTDOWN_PIN);
}

void power_down() {
	// send shutdown to raspberry pi
	gpio_clear(SHUTDOWN_PORT, SHUTDOWN_PIN);
	// wait 20 seconds for raspberry pi to shutdown
	wait(20000);
	// disable power board
	gpio_clear(POWER_PORT, POWER_PIN);
	// enter in standby mode
	standby();
}

/* IRQ */

void tim2_isr(void)
{
	uint16_t led_oc;
	if (timer_get_flag(TIM2, TIM_SR_CC1IF)) {

		/* Clear compare interrupt flag. */
		timer_clear_flag(TIM2, TIM_SR_CC1IF);

		switch (cur_led_mode) {
		case LED_MODE_OFF:
			cur_led_oc = 0;
			led_oc = 0;
			break;
		case LED_MODE_SOFT_BLINK:
			if (cur_led_dir) {
				if (cur_led_oc < LED_MAX_OC) {
					cur_led_oc += PWM_INCR;
				} else {
					cur_led_dir = 0;
				}

			} else {
				if (cur_led_oc > 0) {
					cur_led_oc -= PWM_INCR;
				} else {
					cur_led_dir = 1;
				}
			}
			led_oc = cur_led_oc;

			break;
		case LED_MODE_ON:
			cur_led_oc = LED_MAX_OC;
			led_oc = LED_MAX_OC;
			break;
		case LED_MODE_HARD_BLINK:
			if (cur_led_oc < LED_MAX_OC / 2) {
				if ((cur_led_oc / 10) % 2) {
					led_oc = LED_MAX_OC;
				} else {
					led_oc = 0;
				}
			} else {
				led_oc = 0;
			}
			cur_led_oc++;
			if (cur_led_oc > LED_MAX_OC)
				cur_led_oc = 0;
			break;
		default:
			cur_led_oc = 0;
			led_oc = 0;
			break;
		}

		led_pwm_set_dc(PWM_TIMER_R, PWM_CHAN_R, (int)((float)led_oc * ((float)led_r / 255.0) * 0.5 * led_lum));
		led_pwm_set_dc(PWM_TIMER_G, PWM_CHAN_G, (int)((float)led_oc * ((float)led_g / 255.0) * 0.75 * led_lum));
		led_pwm_set_dc(PWM_TIMER_B, PWM_CHAN_B, (int)((float)led_oc * ((float)led_b / 255.0) * 1.0 * led_lum));
	} else {
		led_pwm_set_dc(PWM_TIMER_R, PWM_CHAN_R, 0);
		led_pwm_set_dc(PWM_TIMER_G, PWM_CHAN_G, 0);
		led_pwm_set_dc(PWM_TIMER_B, PWM_CHAN_B, 0);
	}
}

void rtc_isr(void)
{
	started = false;
	led_set_mode(LED_MODE_HARD_BLINK);
	wait(1000);
	power_down_event = 1;
	rtc_disable_alarm();
	exti_reset_request(EXTI17);
}

void exti0_1_isr(void)
{
	power_down_postpone();
	exti_reset_request(EXTI0);
}


void exti4_15_isr(void)
{
	started = true;
	power_down_postpone();
	exti_reset_request(EXTI5);
}

int main(void)
{
	started = false;
	cur_bat_v = 4.2;
	clock_setup();
	rtc_setup();
	gpio_setup();
	gpio_irq_setup();
	adc_setup();

//	led_pwm_setup(PWM_RCC, PWM_TIMER, PWM_CHAN, PWM_PORT, PWM_PIN, PWM_AF, TIM_OCM_PWM1);

	led_pwm_init(PWM_RCC_R, PWM_TIMER_R);
	led_pwm_init(PWM_RCC_G, PWM_TIMER_G);
	led_pwm_init(PWM_RCC_G, PWM_TIMER_B);

	led_pwm_setup(PWM_TIMER_R, PWM_CHAN_R, PWM_PORT_R, PWM_PIN_R, PWM_AF_R, TIM_OCM_PWM2);
	led_pwm_setup(PWM_TIMER_G, PWM_CHAN_G, PWM_PORT_G, PWM_PIN_G, PWM_AF_G, TIM_OCM_PWM2);
	led_pwm_setup(PWM_TIMER_B, PWM_CHAN_B, PWM_PORT_B, PWM_PIN_B, PWM_AF_B, TIM_OCM_PWM2);

	led_set_luminance(0.5);
	led_set_color(255, 255, 255);

	led_timer_setup();

	led_set_mode(LED_MODE_SOFT_BLINK);

	/* enable rtc irq */
	nvic_enable_irq(NVIC_RTC_IRQ);
	nvic_set_priority(NVIC_RTC_IRQ, 1);

	/* EXTI line 17 is connected to the RTC Alarm event */
	exti_set_trigger(EXTI17, EXTI_TRIGGER_RISING);
	exti_enable_request(EXTI17);

	power_down_event = 0;
	power_up();
	power_down_postpone();

	while (1) {
		if(started) {
			led_set_mode(LED_MODE_ON);
		}
		if (gpio_get(GPIOA, GPIO0) == 1) {
			power_down_postpone();
		}
		if (power_down_event == 1) {
			rcc_disable_rtc_clock();
			exti_disable_request(EXTI17);
			nvic_disable_irq(NVIC_RTC_IRQ);
			power_down();
		}

		cur_bat_v = bat_sense();
		uint8_t cur_bat_state = ((cur_bat_v - 1.0f) / 0.3125f) * 255;
		led_set_color(255 - cur_bat_state, cur_bat_state, 0);

		wait(1000);
	}

	/* should not be here */
}
#endif