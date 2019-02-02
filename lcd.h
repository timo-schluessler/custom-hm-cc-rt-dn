#ifndef LCD_INCLUDED
#define LCD_INCLUDED

#define LCD_DEG_1 0
#define LCD_DEG_2 7
#define LCD_DEG_3 14
#define LCD_TIME_1 21
#define LCD_TIME_2 28
#define LCD_TIME_3 35
#define LCD_TIME_4 42


#include <stdint.h>
#include <stdbool.h>

#include "stm8l.h"

void lcd_init();
void lcd_set_seg(uint8_t seg, bool set);
void lcd_set_digit(uint8_t digit, uint8_t value);
void lcd_clear();
void lcd_sync();

inline void lcd_enable_sync_interrupt()
{
	LCD_CR3 |= LCD_CR3_SOFIE | LCD_CR3_SOFC; // enable lcd interrupt and clear flag to update ui on next sync
}
inline void lcd_disable_sync_interrupt()
{
	LCD_CR3 &= ~LCD_CR3_SOFIE; // disable lcd interrupt
}

#endif
