#include "lcd_defines.h"

uint8_t wanted_heat; // 0-50 = 0.0 - 5.0

#define UI_WAIT 2000

#define WHEELS (WHEEL_A | WHEEL_B)
uint8_t last_wheel = 0, last_wheel_in;
uint8_t wheel = 0;

void ui_init()
{
	PF_CR1 |= BUTTON_LEFT | BUTTON_MIDDLE | BUTTON_RIGHT; // enable pullups
	PF_CR2 = BUTTON_LEFT | BUTTON_MIDDLE | BUTTON_RIGHT; // enable interrupts
	EXTI_CR3 = (0b11*EXTI_CR3_PFIS); // use EXTIF for button edges
	EXTI_CONF1 = EXTI_CONF1_PFES; // use PORTF for EXTIEF interrupt generation

	PC_CR2 = WHEEL_A; // enable external interrupt
	EXTI_CR1 = (0b11*EXTI_CR1_P1IS); // use EXTI1 for wheel edges

	last_wheel_in = PC_IDR & WHEELS;
}

void ui_deinit()
{
	PC_CR2 = 0; // disable wheel interrupts
	PF_CR2 = 0; // disable button interrupts
}

bool in_ui_wait = false;
void ui_wait()
{
	in_ui_wait = true;
	set_timeout(get_tick() + UI_WAIT);
	wait_timeout();
	in_ui_wait = false;
}

bool in_ui_update = false;
bool need_ui_update = false;
void ui_update()
{
ui_update_start:
	lcd_sync();
	need_ui_update = false;
	in_ui_update = true;

	{
		uint8_t cur_wheel = wheel; // only read this once
		wanted_heat += (int8_t)(cur_wheel - last_wheel);
		last_wheel = cur_wheel;
		if (wanted_heat > 128)
			wanted_heat = 0;
		else if (wanted_heat > 50)
			wanted_heat = 50;
	}
	
	{
		uint8_t sub = wanted_heat % 10;
		uint8_t main = wanted_heat / 10;

		lcd_set_digit(LCD_DEG_1, 0x10);
		lcd_set_digit(LCD_DEG_2, main);
		lcd_set_digit(LCD_DEG_3, sub);
		lcd_set_seg(LCD_SEG_DEG_2_DOT, true);
	}

	{
		uint16_t value;
		uint8_t digit[4];
		bool all_zero = true;
		if ((PF_IDR & BUTTON_MIDDLE) == 0) // when middle button is pressed display motor_position
			value = motor_position;
		else
			value = motor_percent;

		for (uint8_t i = 0; i < 4; i++) {
			digit[i] = value % 10;
			value /= 10;
		}
		for (uint8_t i = 0; i < 4; i++) {
			if (all_zero && digit[3-i] == 0 && i != 3)
				digit[3-i] = 0x10;
			else
				all_zero = false;
			lcd_set_digit(LCD_TIME_1 + i * 7, digit[3-i]);
		}
	}

	in_ui_update = false;
	if (need_ui_update)
		goto ui_update_start;
	if (in_ui_wait)
		set_timeout(get_tick() + UI_WAIT);
}

// EXTI1
void wheel_a_isr() __interrupt(9)
{
	uint8_t cur_wheel;

	EXTI_SR1 = EXTI_SR1_P1F;
	cur_wheel = PC_IDR & WHEELS;

	if (cur_wheel == last_wheel_in)
		return;
	if ((cur_wheel ^ (cur_wheel >> 1)) & 1)
		wheel++;
	else
		wheel--;
	
	last_wheel_in = cur_wheel;

	if (!in_ui_update)
		ui_update();
	else
		need_ui_update = true;

	/* one direction
		01 -> 10
		10 -> 01
	*/
	/* the other
		00 -> 11
		11 -> 00
	*/
}

// EXTIF
void button_isr() __interrupt(5)
{
	EXTI_SR2 = EXTI_SR2_PFF;

	if (!in_ui_update)
		ui_update();
	else
		need_ui_update = true;
}










#if 0
// this code nicely works to count on each edge. but for the encoder "ratches" only on each second count, we don't use this code
void wheeled()
{
	uint8_t wheel = PC_IDR & WHEELS;

	/* one direction
		00 -> 01 : 01 ^: 10
		01 -> 11 : 10 ^: 00
		11 -> 10 : 01 ^: 10
		10 -> 00 : 10 ^: 00
	*/
	/* the other
		00 -> 10 : 10 ^: 11
		10 -> 11 : 01 ^: 01
		11 -> 01 : 10 ^: 11
		01 -> 00 : 01 ^: 01
	*/
	
#if 0
	uint8_t change = wheel ^ last_wheel;
	uint8_t edge;
	if (change == 0)
		return;
	edge = (wheel >> (change - 1)); // rising or falling?
	if (((wheel >> (2 - change)) ^ edge ^ change) & 1)
		lcd_data.value++;
	else
		lcd_data.value--;
#else
	if (wheel == last_wheel)
		return;
	//if ((wheel ^ ((wheel << 1) | (wheel >> 1)) ^ change) & 1) // wheel << 1 has no effect because of the & 1
	//if ((wheel ^ (wheel >> 1) ^ change) & 1) // substitute change. and wheel ^ wheel = 0
	if ((last_wheel ^ (wheel >> 1)) & 1)
		lcd_data.value++;
	else
		lcd_data.value--;
#endif

	lcd_data.value &= 0xf;

	last_wheel = wheel;
}

// EXTI0
void wheel_b_isr() __interrupt(8)
{
	EXTI_SR1 = EXTI_SR1_P0F;
	wheeled();
}

// EXTI1
void wheel_a_isr() __interrupt(9)
{
	EXTI_SR1 = EXTI_SR1_P1F;
	wheeled();
}
#endif


