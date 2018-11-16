#include "rtc.h"

#include <stdint.h>
#include <stdbool.h>

#include "stm8l.h"

void rtc_init()
{
	// enable clock for rtc
	CLK_PCKENR2 |= CLK_PCKENR2_RTC;

	// disable write protection
	RTC_WPR = 0xca;
	RTC_WPR = 0x53;

	RTC_CR2 = 0; // disable wakeup timer
	while (!(RTC_ISR1 & RTC_ISR1_WUTWF))
		;
	RTC_ISR1 = RTC_ISR1_INIT; // enter initialization
	while (!(RTC_ISR1 & RTC_ISR1_INITF))
		;
	RTC_CR1 = (0b100*RTC_CR1_WUCKSEL); // WUT tick is 1s
	RTC_CR3 = 0;
	RTC_ISR1 = 0; // leave initialization
	// default prescaler values are perfectly good for us
	// TODO do we have to leave init mode???
}

void rtc_sleep(uint16_t seconds)
{
	RTC_WUTRH = seconds >> 8;
	RTC_WUTRL = seconds & 0xff;
	RTC_CR2 = RTC_CR2_WUTIE | RTC_CR2_WUTE; // enable wake up timer and interrupt

	lcd_clear();
	lcd_sync(); // wait until the cleared content is actually written
	LCD_CR3 &= ~LCD_CR3_LCDEN;
	CLK_PCKENR2 &= ~CLK_PCKENR2_LCD; // disable LCD clock
	CLK_PCKENR1 &= ~CLK_PCKENR1_TIM2; // disable TIM2 clock

	__asm__ ("halt\n");
	//while (!(RTC_ISR2 & RTC_ISR2_WUTF))
	//	;
	//RTC_CR2 = 0; // disable wake up timer and interrupt
	//RTC_ISR2 = (uint8_t)~RTC_ISR2_WUTF; // reset WUTF flag

	RTC_CR2 = 0; // disable wake up timer and interrupt

	CLK_PCKENR2 |= CLK_PCKENR2_LCD; // enable LCD clock
	CLK_PCKENR1 |= CLK_PCKENR1_TIM2; // enable TIM2 clock
	LCD_CR3 |= LCD_CR3_LCDEN;
}

void rtc_isr() __interrupt(4)
{
	RTC_ISR2 = (uint8_t)~RTC_ISR2_WUTF; // reset WUTF flag
}

