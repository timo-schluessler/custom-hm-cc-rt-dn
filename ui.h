#ifndef UI_INCLUDED
#define UI_INCLUDED

void ui_init();
void ui_deinit();
void ui_update();
void ui_wait();
extern uint8_t wheel;
extern volatile uint16_t ui_wait_until;
uint8_t wanted_heat; // 0-50 = 0.0 - 5.0

#define UI_WAIT 2000

#endif
