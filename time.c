#include "time.h"

#include "stm8l.h"

uint16_t get_tick()
{
	uint16_t cur_tick = ((uint16_t)TIM2_CNTRH << 8) + TIM2_CNTRL;
	return cur_tick;
}

void set_timeout(uint16_t timeout)
{
	uint16_t now = get_tick();
	now += timeout;

	TIM2_CCR1H = now >> 8; // set new capture compare value
	TIM2_CCR1L = now & 0xff;
	TIM2_SR1 = ~TIMx_SR1_CC1IF; // reset capture compare flag
	TIM2_IER = TIMx_IER_CC1IE; // enable interrupt
}

void clear_timeout()
{
	TIM2_IER = 0; // disable interrupt
}
