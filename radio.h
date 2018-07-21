#ifndef RADIO_INCLUDED
#define RADIO_INCLUDED

#include "as.h"

void radio_init();
void radio_poll();
void radio_enter_receive(uint8_t max_length);
bool radio_receive(as_packet_t * pkg, uint8_t max_length);

#endif
