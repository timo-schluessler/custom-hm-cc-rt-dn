
#include <stdint.h>
#include <stdbool.h>

#include "stm8l.h"

#include "lcd.h"

#define LEDS (1u<<7)

#define MOTOR_LEFT_HIGH (1u<<4) // PD4
#define MOTOR_LEFT_LOW (1u<<6) // PD6
#define MOTOR_RIGHT_HIGH (1u<<5) // PD5
#define MOTOR_RIGHT_LOW (1u<<7) // PD7
#define MOTORS_MASK (MOTOR_LEFT_HIGH | MOTOR_LEFT_LOW | MOTOR_RIGHT_HIGH | MOTOR_RIGHT_LOW)

volatile uint8_t addr = 0;
volatile uint8_t bit = 0;
volatile uint16_t cur_tick;

void lcd_test();
void motor_test();

//#define USE_LSE

#define tick_elapsed(tick) (!((get_tick() - tick) & ((uint16_t)1<<15)))
uint16_t get_tick()
{
	cur_tick = ((uint16_t)TIM2_CNTRH << 8) + TIM2_CNTRL;
	return cur_tick;
}

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
#else
	// clock divider to 128
	CLK_CKDIVR = 0b111;
	// enable low speed external manually (it's needed for RTC clk which is needed for LCD clk)
	CLK_ECKCR |= CLK_ECKCR_LSEON;
	while (!(CLK_ECKCR & CLK_ECKCR_LSERDY))
		;
#endif

	// configure RTC clock
	CLK_CRTCR = (0b1000<<CLK_CRTCR_RTCSEL); // use LSE as RTC clk source
	while (CLK_CRTCR & CLK_CRTCR_RTCSWBSY) // wait for the switch to finish
		;
	CLK_PCKENR2 |= CLK_PCKENR2_LCD; // enable LCD clock
	LCD_FRQ = (3<<4) | 0; // PS[3:0] = 3, DIV[3:0] = 0 -> clock divier = 128 -> f_LCD = 128Hz, f_frame = 32Hz.
	LCD_CR1 = (0b11<<1) | 0; // 1/4 duty, 1/3 bias
	LCD_CR2 = (0b000<<5) | (0<<4) | (0b000<<1) | (0<<0); // PON = 0, HD = 0, CC = 0, VSEL = 0
	LCD_CR3 = (0b000<<0); // DEAD = 0
	LCD_PM0 = 0b11111111; // use segments 00 - 07
	LCD_PM1 = 0b00011111; // use segments 08 - 12
	LCD_PM2 = 0b11000000; // use segments 22 - 23
	LCD_PM3 = 0b11111111; // use segments 24 - 31
	LCD_PM4 = 0b00001111; // use segments 32 - 35 (36 - 39 is in fact COM4 - COM7 which are GPIOs)
	LCD_CR4 = 0; // no duty8

	LCD_CR3 |= LCD_CR3_LCDEN;

	// enable tick counter
	CLK_PCKENR1 |= CLK_PCKENR1_TIM2;
	SYSCFG_RMPCR2 |= SYSCFG_RMPCR2_TIM2TRIGLSE_REMAP; // remap LSE to tim2 trigger input
	TIM2_SMCR = (0b111<<TIMx_SMCR_TS) | (0b111<<TIMx_SMCR_SMS); // external clock mode 1
	TIM2_ETR = (0b11<<TIMx_ETR_ETPS); // etr prescaler = 8
	TIM2_PSCR = 2; // prescale by 2^2 = 4. one tick is 1/1024s
	TIM2_EGR = 1; // force update to make all changes take effect immediately
	TIM2_CR1 = TIMx_CR1_CEN;
	
	PF_CR1 = LEDS; // push-pull
	PF_DDR = LEDS; // output
	//PF_ODR |= LEDS; // enable backlight

	{
		PD_CR1 = MOTORS_MASK; // push-pull
		PD_DDR = MOTORS_MASK; // output
		PD_ODR = MOTOR_LEFT_HIGH | MOTOR_RIGHT_HIGH; // all BJTs off
	}


	while (true) {
		lcd_test();
		motor_test();
	}
}


uint16_t timeout_lcd = 0;
uint8_t value = 0;
uint8_t last_value = 0;
void lcd_test()
{
	//if (tick_elapsed(timeout_lcd)) {
		//timeout_lcd += 1024;
	if (value != last_value) {

		lcd_sync();
		lcd_set_digit(LCD_DEG_1, value);
		lcd_set_digit(LCD_DEG_2, value);
		lcd_set_digit(LCD_DEG_3, value);

		lcd_set_digit(LCD_TIME_1, value);
		lcd_set_digit(LCD_TIME_2, value);
		lcd_set_digit(LCD_TIME_3, value);
		lcd_set_digit(LCD_TIME_4, value);
		last_value = value;
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
			set_motor(HIGH, LOW);
		else
			set_motor(LOW, HIGH);
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
	value = state;
}
