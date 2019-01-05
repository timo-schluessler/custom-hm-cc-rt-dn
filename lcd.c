#include "lcd.h"

#include "lcd_defines.h"

#include "stm8l.h"

#include <stdbool.h>

void lcd_init()
{
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
}


CONSTMEM uint8_t lcd_digits[] = {
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

CONSTMEM uint8_t sev_seg_codes[] = {
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
	0b0011011,
	0b0000000 // 0x10 is clear/empty digit
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

void lcd_clear()
{
	lcd_sync();
	for (uint8_t i = 0; i < LCD_RAM_SIZE; i++)
		LCD_RAM[i] = 0;
}

void lcd_sync()
{
	LCD_CR3 |= LCD_CR3_SOFC;
	while (!(LCD_CR3 & LCD_CR3_SOF))
		;
}

