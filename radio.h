#ifndef RADIO_INCLUDED
#define RADIO_INCLUDED

#include "as.h"

void radio_init();
void radio_deinit();
void radio_poll();

bool radio_wait(uint16_t timeout);

void radio_enter_receive(uint8_t max_length);
bool radio_received();
bool radio_receive(as_packet_t * pkg, uint8_t max_length);

void radio_send(as_packet_t * pkg);
bool radio_sent();

#endif
