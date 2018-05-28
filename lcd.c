#include "lcd.h"

#include "lcd_defines.h"

#include "stm8l.h"

#include <stdbool.h>

const uint8_t lcd_digits[] = {
	LCD_SEG_DEG_1_SEG_1,
	LCD_SEG_DEG_1_SEG_2,
	LCD_SEG_DEG_1_SEG_3,
	LCD_SEG_DEG_1_SEG_4,
	LCD_SEG_DEG_1_SEG_5,
	LCD_SEG_DEG_1_SEG_6,
	LCD_SEG_DEG_1_SEG_7,

	LCD_SEG_DEG_2_SEG_1,
	LCD_SEG_DEG_2_SEG_2,
	LCD_SEG_DEG_2_SEG_3,
	LCD_SEG_DEG_2_SEG_4,
	LCD_SEG_DEG_2_SEG_5,
	LCD_SEG_DEG_2_SEG_6,
	LCD_SEG_DEG_2_SEG_7,

	LCD_SEG_DEG_3_SEG_1,
	LCD_SEG_DEG_3_SEG_2,
	LCD_SEG_DEG_3_SEG_3,
	LCD_SEG_DEG_3_SEG_4,
	LCD_SEG_DEG_3_SEG_5,
	LCD_SEG_DEG_3_SEG_6,
	LCD_SEG_DEG_3_SEG_7,

	LCD_SEG_TIME_1_SEG_1,
	LCD_SEG_TIME_1_SEG_2,
	LCD_SEG_TIME_1_SEG_3,
	LCD_SEG_TIME_1_SEG_4,
	LCD_SEG_TIME_1_SEG_5,
	LCD_SEG_TIME_1_SEG_6,
	LCD_SEG_TIME_1_SEG_7,

	LCD_SEG_TIME_2_SEG_1,
	LCD_SEG_TIME_2_SEG_2,
	LCD_SEG_TIME_2_SEG_3,
	LCD_SEG_TIME_2_SEG_4,
	LCD_SEG_TIME_2_SEG_5,
	LCD_SEG_TIME_2_SEG_6,
	LCD_SEG_TIME_2_SEG_7,

	LCD_SEG_TIME_3_SEG_1,
	LCD_SEG_TIME_3_SEG_2,
	LCD_SEG_TIME_3_SEG_3,
	LCD_SEG_TIME_3_SEG_4,
	LCD_SEG_TIME_3_SEG_5,
	LCD_SEG_TIME_3_SEG_6,
	LCD_SEG_TIME_3_SEG_7,

	LCD_SEG_TIME_4_SEG_1,
	LCD_SEG_TIME_4_SEG_2,
	LCD_SEG_TIME_4_SEG_3,
	LCD_SEG_TIME_4_SEG_4,
	LCD_SEG_TIME_4_SEG_5,
	LCD_SEG_TIME_4_SEG_6,
	LCD_SEG_TIME_4_SEG_7
};

void lcd_set_seg(uint8_t seg, bool set)
{
	uint8_t off = seg >> 3;
	uint8_t bits = 1<<(seg & 0x7);
	if (set)
		LCD_RAM[off] |= bits;
	else
		LCD_RAM[off] &= ~bits;
}

const uint8_t sev_seg_codes[] = {
	0b1110111,
	0b0100100,
	0b1011101,
	0b1101101,
	0b0101110,
	0b1101011,
	0b1111011,
	0b0100111,
	0b1111111,
	0b1101111,
	0b0111111,
	0b1111010,
	0b1010011,
	0b1111100,
	0b1011011,
	0b0011011
};

void lcd_set_digit(uint8_t digit, uint8_t value)
{
	uint8_t c = sev_seg_codes[value];
	uint8_t const * digits = lcd_digits + digit;

	for (int i = 0; i < 7; i++) {
		lcd_set_seg(*(digits++), c & 1);
		c >>= 1;
	}
}

void lcd_sync()
{
	LCD_CR3 |= LCD_CR3_SOFC;
	while (!(LCD_CR3 & LCD_CR3_SOF))
		;
}
