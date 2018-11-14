#include "time.h"

#include "stm8l.h"

void tick_init()
{
	// enable tick counter
	CLK_PCKENR1 |= CLK_PCKENR1_TIM2;
	SYSCFG_RMPCR2 |= SYSCFG_RMPCR2_TIM2TRIGLSE_REMAP; // remap LSE to tim2 trigger input
	TIM2_SMCR = (0b111<<TIMx_SMCR_TS) | (0b111<<TIMx_SMCR_SMS); // external clock mode 1
	TIM2_ETR = (0b11<<TIMx_ETR_ETPS); // etr prescaler = 8
	TIM2_PSCR = 2; // prescale by 2^2 = 4. one tick is 1/1024s
	TIM2_EGR = 1; // force update to make all changes take effect immediately
	TIM2_CR1 = TIMx_CR1_CEN;

	WFE_CR1 |= WFE_CR1_TIM2_EV1; // timer 2 capture and compare events
}

void tick_deinit()
{
	clear_timeout();
	WFE_CR1 &= ~WFE_CR1_TIM2_EV1; // timer 2 capture and compare events
	CLK_PCKENR1 &= ~CLK_PCKENR1_TIM2;
}

uint16_t get_tick()
{
	return ((uint16_t)TIM2_CNTRH << 8) + TIM2_CNTRL;
}

void set_timeout(uint16_t timeout_at)
{
	TIM2_CCR1H = timeout_at >> 8; // set new capture compare value
	TIM2_CCR1L = timeout_at & 0xff;
	TIM2_SR1 = (uint8_t)(~((uint8_t)TIMx_SR1_CC1IF)); // reset capture compare flag
	TIM2_IER = TIMx_IER_CC1IE; // enable interrupt
}

void clear_timeout()
{
	TIM2_IER = 0; // disable interrupt
}

void wait_timeout()
{
	while (!(TIM2_SR1 & TIMx_SR1_CC1IF))
		wfe();
}
