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
void lcd_set_digit(uint8_t digit, uint8_t value);
void lcd_sync();

#endif
