
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

#define LEDS (1u<<7)

#define BUTTON_LEFT (1u<<4) // PF4
#define BUTTON_MIDDLE (1u<<5) // PF5
#define BUTTON_RIGHT (1u<<6) // PF6

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
	__asm__ ("rim\n"); // disable interrupts

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

	PF_CR1 |= BUTTON_LEFT | BUTTON_MIDDLE | BUTTON_RIGHT; // enable pullups
	EXTI_CR3 = (0b11*EXTI_CR3_PFIS); // use EXTIF for button edges
	EXTI_CONF1 = EXTI_CONF1_PFES; // use PORTF for EXTIEF interrupt generation

	PC_CR2 = WHEEL_A | WHEEL_B; // enable external interrupt
	EXTI_CR1 = (0b11*EXTI_CR1_P0IS) | (0b11*EXTI_CR1_P1IS); // use EXTI0 and EXTI1 for wheel edges

	ADC1_TRIGR1 = ADC_TRIGR1_TRIG24; // disable schmitt trigger for analog temperature input

	motor_init();

	rtc_init();

	radio_init();
	//spi_disable();
	//radio_enter_receive(14);

	//motor_ref();

	while (true) {
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
		//radio_poll();
		//measure_temperature();
		//delay_ms(1000);
		//rtc_sleep(5);
	}
}

#include "si4430.c"

void lcd_test()
{
	//if (tick_elapsed(timeout_lcd)) {
		//timeout_lcd += 1024;
	if (lcd_data.value != lcd_data.last_value) {

		lcd_sync();
		lcd_set_digit(LCD_DEG_1, lcd_data.value);
		lcd_set_digit(LCD_DEG_2, lcd_data.value);
		lcd_set_digit(LCD_DEG_3, lcd_data.value);

		lcd_set_digit(LCD_TIME_1, lcd_data.value);
		lcd_set_digit(LCD_TIME_2, lcd_data.value);
		lcd_set_digit(LCD_TIME_3, lcd_data.value);
		lcd_set_digit(LCD_TIME_4, lcd_data.value);
		lcd_data.last_value = lcd_data.value;
		//if (++value == 0x10)
		//	value = 0;
	}
}

#include "motor.c"

volatile uint16_t temp_test;
volatile float temp;
#include <math.h>
void measure_temperature()
{
	uint32_t sum = 0;

	PF_ODR |= TEMP_SENSOR_OUT;
	CLK_PCKENR2 |= CLK_PCKENR2_ADC1;
#define ADC1_CR1_VALUE (ADC_CR1_ADON | (0b0 * ADC_CR1_RES0)) // enable ad, 12bit resolution
	ADC1_CR1 = ADC1_CR1_VALUE;
	//ADC1_CR2 = 0; // system clock as adc clock, no ext trigger, 4 ADC clocks sampling time
	ADC1_SQR1 = ADC_SQR1_CHSEL_S24; // select channel 24 which is PF0
	ADC1_SR = 0; // reset EOC flag

	for (uint8_t i = 0; ;) {
		ADC1_CR1 = ADC1_CR1_VALUE | ADC_CR1_START; // start conversion
		while (!(ADC1_SR & ADC_SR_EOC))
			;
		sum += ((uint16_t)ADC1_DRH << 8) | ADC1_DRL;
		if (++i == 0) // do the check here at end, so in fact we summed it up 256 times
			break;
	}
	temp_test = sum >> 8;

	PF_ODR &= ~TEMP_SENSOR_OUT;
	ADC1_CR1 = 0; // disable ad
	CLK_PCKENR2 &= ~CLK_PCKENR2_ADC1;

	{
		// TODO use a lookup table of some kind?
#define BETA_INV .0002531645 // 1/3950
#define K25_INV .00335401643468052993 // 1/298.15 = 1/(25°C.) = 1/(25K + 273.15K)
		// theese two lines increase code size by 3.3kb
		//float tmp = temp_test / (float)(4096 - temp_test); // = R / R_0
		//temp = 1.0/(K25_INV + BETA_INV * logf(tmp)) - 273.15;
	}

	// TODO convert battery voltage


}

#include "rtc.c"
#include "ui.c"
