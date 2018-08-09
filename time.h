#ifndef TIME_INCLUDED
#define TIME_INCLUDED

#include <stdint.h>

void tick_init();

#define GET_MACRO(_1,_2,NAME,...) NAME
#define tick_elapsed(...) GET_MACRO(__VA_ARGS__, tick_elapsed2, tick_elapsed1)(__VA_ARGS__)
		
#define tick_elapsed1(tick) (!((get_tick() - (tick)) & ((uint16_t)1<<15)))
#define tick_elapsed2(now, tick) (!(((now) - (tick)) & ((uint16_t)1<<15)))

uint16_t get_tick();

void set_timeout(uint16_t timeout_at);
void clear_timeout();

#endif
