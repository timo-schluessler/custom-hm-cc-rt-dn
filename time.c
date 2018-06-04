#include "time.h"

#include "stm8l.h"

uint16_t get_tick()
{
	uint16_t cur_tick = ((uint16_t)TIM2_CNTRH << 8) + TIM2_CNTRL;
	return cur_tick;
}
