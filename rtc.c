#include "rtc.h"

#include <stdint.h>
#include <stdbool.h>

#include "stm8l.h"

void rtc_init()
{
	// disable write protection
	RTC_WPR = 0xca;
	RTC_WPR = 0x53;

	RTC_ISR1 = RTC_ISR1_INIT; // enter initialization
	RTC_CR2 = 0; // disable wakeup timer
	while (!(RTC_ISR1 & RTC_ISR1_INITF) || !(RTC_ISR1 & RTC_ISR1_WUTWF))
		;
	RTC_CR1 = (0b100*RTC_CR1_WUCKSEL); // WUT tick is 1s
	RTC_CR3 = 0;
	// default prescaler values are perfectly good for us
	// TODO do we have to leave init mode???
}

void rtc_sleep(uint16_t seconds)
{
	RTC_WUTRH = seconds >> 8;
	RTC_WUTRL = seconds & 0xff;
	RTC_CR2 = RTC_CR2_WUTIE | RTC_CR2_WUTE; // enable wake up timer and interrupt

	//halt();
	while (!(RTC_ISR2 & RTC_ISR2_WUTF))
		;
	RTC_CR2 = 0; // enable wake up timer and interrupt
	RTC_ISR2 = ~(1<<RTC_ISR2_WUTF); // reset WUTF flag

	// TODO??
	/*// configure RTC clock
	CLK_CRTCR = (0b1000<<CLK_CRTCR_RTCSEL); // use LSE as RTC clk source
	while (CLK_CRTCR & CLK_CRTCR_RTCSWBSY) // wait for the switch to finish
		;*/
}
