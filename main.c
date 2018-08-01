
#include <stdint.h>
#include <stdbool.h>

#include "stm8l.h"

#include "lcd.h"
#include "radio.h"
#include "delay.h"

#define LEDS (1u<<7)

#define MOTOR_LEFT_HIGH (1u<<4) // PD4
#define MOTOR_LEFT_LOW (1u<<6) // PD6
#define MOTOR_RIGHT_HIGH (1u<<5) // PD5
#define MOTOR_RIGHT_LOW (1u<<7) // PD7
#define MOTORS_MASK (MOTOR_LEFT_HIGH | MOTOR_LEFT_LOW | MOTOR_RIGHT_HIGH | MOTOR_RIGHT_LOW)

void lcd_test();
void motor_test();

//#define USE_LSE

#include "time.h"

void main()
{
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
	// clock divider to 128
	//CLK_CKDIVR = 0b111; // TODO for test only
	// enable low speed external manually (it's needed for RTC clk which is needed for LCD clk)
	CLK_ECKCR |= CLK_ECKCR_LSEON;
	while (!(CLK_ECKCR & CLK_ECKCR_LSERDY))
		;
	// f_sysclk = 16MHz / 128 = 125kHz
#endif

	// configure RTC clock
	CLK_CRTCR = (0b1000<<CLK_CRTCR_RTCSEL); // use LSE as RTC clk source
	while (CLK_CRTCR & CLK_CRTCR_RTCSWBSY) // wait for the switch to finish
		;

	lcd_init();
	tick_init();

	PF_CR1 = LEDS; // push-pull
	PF_DDR = LEDS; // output
	//PF_ODR |= LEDS; // enable backlight

	{
		PD_CR1 = MOTORS_MASK; // push-pull
		PD_DDR = MOTORS_MASK; // output
		PD_ODR = MOTOR_LEFT_HIGH | MOTOR_RIGHT_HIGH; // all BJTs off
	}

	radio_init();
	radio_enter_receive(14);

	while (true) {
		lcd_test();
		//motor_test();
		radio_poll();
	}
}

typedef struct {
	uint16_t timeout_lcd;
	uint8_t value;
	uint8_t last_value;
} lcd_t;
lcd_t lcd_data = { 0 };

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

#define OFF 0
#define LOW 1
#define HIGH 2
void set_motor(int8_t left, int8_t right)
{
	uint8_t odr = PD_ODR & ~MOTORS_MASK;

	if (left == OFF)
		odr |= MOTOR_LEFT_HIGH;
	else if (left == LOW)
		odr |= MOTOR_LEFT_LOW | MOTOR_LEFT_HIGH;
	//else if (left == HIGH)
	//	odr |= 0;
	
	if (right == OFF)
		odr |= MOTOR_RIGHT_HIGH;
	else if (right == LOW)
		odr |= MOTOR_RIGHT_LOW | MOTOR_RIGHT_HIGH;
	//else if (left == HIGH)
	//	odr |= 0;

	PD_ODR = odr;
}

uint16_t timeout = 1024;
enum { Stop, Turn, CrossOver, Recirculate } state = Stop;
bool dir = false;

void motor_test()
{

	if (!tick_elapsed(timeout))
		return;
	
	if (state == Stop) {
		state = Turn;
		timeout += 5 * 1024;
		if (dir)
			;//set_motor(HIGH, LOW);
		else
			;//set_motor(LOW, HIGH);
	}
	else if (state == Turn) {
		state = CrossOver;
		timeout += 20;
		if (dir)
			set_motor(OFF, LOW);
		else
			set_motor(LOW, OFF);
	}
	else if (state == CrossOver) {
		set_motor(LOW, LOW);
		state = Recirculate;
		timeout += 500;
	}
	else if (state == Recirculate) {
		state = Stop;
		set_motor(OFF, OFF);

#if 1
		// TODO debug
		timeout += 2000;
		dir = !dir;
#else
		timeout += 10; // 10ms before turning on again
#endif
	}
	lcd_data.value = state;
}
