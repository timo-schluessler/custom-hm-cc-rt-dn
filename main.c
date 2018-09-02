
#include <stdint.h>
#include <stdbool.h>

#include "stm8l.h"

#define F_CPU 125000UL

#define MAX_PAYLOAD 10

#include "lcd.h"
#include "radio.h"
#include "delay.h"
#include "spi.h"
#include "motor.h"

#define LEDS (1u<<7)

#define BUTTON_LEFT (1u<<4)
#define BUTTON_MIDDLE (1u<<5)
#define BUTTON_RIGHT (1u<<6)

void lcd_test();

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
	CLK_CKDIVR = 0b111; // TODO for test only
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

	PF_CR1 |= BUTTON_LEFT | BUTTON_MIDDLE | BUTTON_RIGHT; // enable pullups

	motor_init();

	radio_init();
	spi_disable();
	//radio_enter_receive(14);

	motor_ref();

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
		if ((PF_IDR & BUTTON_MIDDLE) == 0)
			motor_move_to(2);
		if ((PF_IDR & BUTTON_RIGHT) == 0)
			motor_move_to(100);
		//radio_poll();
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

#include "motor.c"

