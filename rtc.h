#ifndef RTC_INCLUDED
#define RTC_INCLUDED

void rtc_init();
void rtc_sleep(uint16_t seconds);

extern bool rtc_caused_wakeup;

#endif
