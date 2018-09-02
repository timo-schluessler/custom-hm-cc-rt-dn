#ifndef MOTOR_INCLUDED
#define MOTOR_INCLUDED

typedef int16_t motor_position_t;

void motor_init();
void motor_ref();
void motor_move_to(uint8_t percent);

#endif
