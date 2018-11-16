
#include <stdint.h>
#include <stdbool.h>

#include "stm8l.h"

#define F_CPU 500000UL

#define MAX_PAYLOAD 60

#include "lcd.h"
#include "radio.h"
#include "delay.h"
#include "spi.h"
#include "motor.h"
#include "rtc.h"
#include "ui.h"

#define LEDS (1u<<7)

#define BUTTON_LEFT (1u<<4) // PF4
#define BUTTON_MIDDLE (1u<<5) // PF5
#define BUTTON_RIGHT (1u<<6) // PF6
#define BUTTONS (BUTTON_LEFT | BUTTON_MIDDLE | BUTTON_RIGHT)


#define WHEEL_A (1u<<1) // PC1
#define WHEEL_B (1u<<0) // PC0

#define TEMP_SENSOR_OUT (1u<<1) // PF1

void lcd_test();
void measure_temperature();

//#define USE_LSE

#include "time.h"

typedef struct {
	uint16_t timeout_lcd;
	uint8_t value;
	uint8_t last_value;
} lcd_t;
lcd_t lcd_data = { 0 };

void main()
{
	disable_interrupts();

	// low speed external clock prevents debugger from working :(
#ifdef USE_LSE
	CLK_SWCR |= CLK_SWCR_SWEN;
	CLK_SWR = CLK_SWR_LSE; // LSE oscillator

	while (CLK_SWCR & CLK_SWCR_SWBSY)
		;

	// clock divier to 1
	CLK_CKDIVR = 0x0;
	
	// stop HSI
	CLK_ICKCR &= CLK_ICKCR_HSION;

	// f_sysclk = 32768Hz
#else
	// clock divider to 32
	CLK_CKDIVR = 0b101;
	// enable low speed external manually (it's needed for RTC clk which is needed for LCD clk)
	CLK_ECKCR |= CLK_ECKCR_LSEON;
	while (!(CLK_ECKCR & CLK_ECKCR_LSERDY))
		;
	// f_sysclk = 16MHz / 128 = 500kHz // must be higher than 320kHz for ADC1!
#endif

	// configure RTC clock
	CLK_CRTCR = (0b1000<<CLK_CRTCR_RTCSEL); // use LSE as RTC clk source
	while (CLK_CRTCR & CLK_CRTCR_RTCSWBSY) // wait for the switch to finish
		;

	lcd_init();
	tick_init();

	PF_CR1 = LEDS | TEMP_SENSOR_OUT; // push-pull
	PF_DDR = LEDS | TEMP_SENSOR_OUT; // output
	//PF_ODR |= LEDS; // enable backlight

	ADC1_TRIGR1 = ADC_TRIGR1_TRIG24; // disable schmitt trigger for analog temperature input

	motor_init();
	rtc_init();
	radio_init();
	//spi_disable(); // TODO
	//radio_enter_receive(14);
	ui_init();

	enable_interrupts();

	//motor_ref();

	while (true) {
		measure_temperature();
		ui_update();

		ui_wait();

		rtc_sleep(10);

#if 0
		lcd_test();
		if ((PF_IDR & BUTTON_LEFT) == 0) {
			for (;;) {
				volatile uint8_t A = PF_IDR;
				A &= BUTTON_LEFT;
				if (A)
					break;
			}
			motor_move_to(0);
		}
		if ((PF_IDR & BUTTON_MIDDLE) == 0) {
			delay_ms(10);
			as_listen();
			//motor_move_to(2);
		}
		if ((PF_IDR & BUTTON_RIGHT) == 0) {
			//motor_move_to(100);
			lcd_data.value++;
			as_send_device_info();
			as_listen();
			delay_ms(10);
		}
#endif
		//radio_poll();
		//measure_temperature();
		//delay_ms(1000);
		//rtc_sleep(5);
	}
}

void main_deinit()
{
	radio_deinit();
	tick_deinit();
	ui_deinit();
}

#include "si4430.c"

void lcd_test()
{
	//if (tick_elapsed(timeout_lcd)) {
		//timeout_lcd += 1024;
	uint8_t value;
	lcd_data.value = wheel & 0xf;
	value = lcd_data.value;
	if (value != lcd_data.last_value) {

		lcd_sync();
		lcd_set_digit(LCD_DEG_1, value);
		lcd_set_digit(LCD_DEG_2, value);
		lcd_set_digit(LCD_DEG_3, value);

		lcd_set_digit(LCD_TIME_1, value);
		lcd_set_digit(LCD_TIME_2, value);
		lcd_set_digit(LCD_TIME_3, value);
		lcd_set_digit(LCD_TIME_4, value);
		lcd_data.last_value = value;
		//if (++value == 0x10)
		//	value = 0;
	}
}

#include "motor.c"

float temp;
uint8_t battery_voltage;
volatile uint16_t duration, duration1;
#include <math.h>
void measure_temperature()
{
	uint16_t sum = 0;

	PF_ODR |= TEMP_SENSOR_OUT;
	CLK_PCKENR2 |= CLK_PCKENR2_ADC1;
	ADC1_TRIGR1 = ADC_TRIGR1_VREFINTON;
#define ADC1_CR1_VALUE (ADC_CR1_ADON | (0b0 * ADC_CR1_RES0)) // enable ad, 12bit resolution
	ADC1_CR1 = ADC1_CR1_VALUE;
	//ADC1_CR2 = 0; // system clock as adc clock, no ext trigger, 4 ADC clocks sampling time
	ADC1_SQR1 = ADC_SQR1_DMAOFF | ADC_SQR1_CHSEL_S24; // select channel 24 which is PF0
	ADC1_SR = 0; // reset EOC flag

	for (uint8_t i = 0; i < 4; i++) {
		ADC1_CR1 = ADC1_CR1_VALUE | ADC_CR1_START; // start conversion
		while (!(ADC1_SR & ADC_SR_EOC))
			;
		sum += ((uint16_t)ADC1_DRH << 8) | ADC1_DRL;
	}
	PF_ODR &= ~TEMP_SENSOR_OUT;

	if (sum & 0x2)
		sum += 0x40;
	sum >>= 2;

	{
		// TODO use a lookup table of some kind?
#define BETA_INV .0002531645 // 1/3950
#define K25_INV .00335401643468052993 // 1/298.15 = 1/(25°C.) = 1/(25K + 273.15K)
		// theese two lines increase code size by 3.3kb
		float tmp = sum / (float)(4096 - sum); // = R / R_0
		temp = 1.0/(K25_INV + BETA_INV * logf(tmp)) - 273.15;
	}

	ADC1_SQR1 = ADC_SQR1_DMAOFF | ADC_SQR1_CHSEL_SVREFINT;

	sum = 0;
	for (uint8_t i = 0; i < 2; i++) {
		ADC1_CR1 = ADC1_CR1_VALUE | ADC_CR1_START; // strat conversion
		while (!(ADC1_SR & ADC_SR_EOC))
			;
		sum += ((uint16_t)ADC1_DRH << 8) | ADC1_DRL;
	}

	ADC1_TRIGR1 = 0; // disable Vrefint
	ADC1_CR1 = 0; // disable ad
	CLK_PCKENR2 &= ~CLK_PCKENR2_ADC1;

	sum >>= 1;
	// factory_conv / 4096 = 1.224 / 3.  // we could use factory conversion to more precisely determine Vrefint. but we don't need it
	//battery_voltage = (1.224 * 4096) / sum;
	battery_voltage = (50135 + (sum >> 1)) / sum;
}

#include "rtc.c"
#include "ui.c"
