#ifndef TIME_INCLUDED
#define TIME_INCLUDED

#include <stdint.h>

#define tick_elapsed(tick) (!((get_tick() - tick) & ((uint16_t)1<<15)))
uint16_t get_tick();

void set_timeout(uint16_t timeout);
void clear_timeout();

#endif
