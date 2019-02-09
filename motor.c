
motor_position_t motor_position;
uint8_t motor_percent;
#define BACKWARD -1
#define FORWARD 1
int8_t motor_dir = BACKWARD;
bool motor_stop = false;

bool motor_run(uint16_t now, bool pulsed);

#define MOTOR_LEFT_HIGH (1u<<4) // PD4 - TIM1_CH2
#define MOTOR_LEFT_LOW (1u<<6) // PD6
#define MOTOR_RIGHT_HIGH (1u<<5) // PD5 - TIM1_CH3
#define MOTOR_RIGHT_LOW (1u<<7) // PD7
#define MOTORS_MASK (MOTOR_LEFT_HIGH | MOTOR_LEFT_LOW | MOTOR_RIGHT_HIGH | MOTOR_RIGHT_LOW)

#define MOTOR_LED (1u<<3)
#define MOTOR_LED_PORT_CR1 PA_CR1
#define MOTOR_LED_PORT_DDR PA_DDR
#define MOTOR_LED_PORT_ODR PA_ODR
#define MOTOR_ENC (1u<<2)
#define MOTOR_ENC_PORT_CR1 PA_CR1
#define MOTOR_ENC_PORT_IDR PA_IDR

void motor_init()
{
	PD_CR1 = MOTORS_MASK; // push-pull
	PD_DDR = MOTORS_MASK; // output
	PD_ODR = MOTOR_LEFT_HIGH | MOTOR_RIGHT_HIGH; // all BJTs off

	MOTOR_LED_PORT_CR1 = MOTOR_LED; // push-pull
	MOTOR_LED_PORT_DDR = MOTOR_LED; // output
}

// enable/disable encoder LED and transistor pullup
#define ENABLE_ENCODER() do { MOTOR_ENC_PORT_CR1 |= MOTOR_ENC; MOTOR_LED_PORT_ODR |= MOTOR_LED; } while(false);
#define DISABLE_ENCODER() do { MOTOR_ENC_PORT_CR1 &= ~MOTOR_ENC; MOTOR_LED_PORT_ODR &= ~MOTOR_LED; } while(false);

#define REF_STALL_THRESHOLD 400
#define MAX_STALL_THRESHOLD 400
#define MOVE_STALL_THRESHOLD 40
#define BREAK_FREE_MARGIN 100
#define START_PULSES 3 // the first rotation may take BREAK_FREE_MARGIN of additional time

#define MEAN_COUNT 12
#define VALVE_START_THRESHOLD 15

#define MEAN2_COUNT 4
#define MEAN2_SHIFT 2
#define MEAN2_THRESHOLD 10
#define MEAN2_HOLD 4

motor_position_t motor_max_pos;
uint8_t motor_error;

uint8_t mem[1400];
uint16_t mem_in = 0;

void motor_ref()
{
	uint16_t lowest;
	uint16_t mean_start;
	uint8_t mean_count;
	uint16_t mean2_start = 0;
	uint8_t mean2_count = 0;
	uint16_t mean2 = 0;
	enum { searching_start, searching_flat, searching_end } state;
	uint8_t state_counter = 0;
	uint8_t start_count;
	uint16_t now;
	uint16_t timeout;
	uint16_t timeout2;
	uint8_t timeouts;
	bool last;
	uint16_t last_t;

	ENABLE_ENCODER();
	motor_error = false;

	lcd_sync();
	lcd_set_digit(LCD_DEG_3, 0);

	// turn backward until we don't see an encoder impuls for longer than x
	motor_dir = BACKWARD;
	start_count = START_PULSES - 1;
	now = get_tick();
	timeout = now + REF_STALL_THRESHOLD;
	timeout2 = now + 30000;
	timeouts = 0;
	last = (bool)(MOTOR_ENC_PORT_IDR & MOTOR_ENC);
	while (true) {
		bool pulsed = false;
		now = get_tick();
		if (tick_elapsed(now, timeout))
			motor_stop = true;
		if (tick_elapsed(now, timeout2)) { // overall timeout
			if (timeouts++ == 0)
				timeout2 += 30000;
			else {
				motor_stop = true;
				motor_error = 1;
			}
		}
		if ((bool)(MOTOR_ENC_PORT_IDR & MOTOR_ENC) != last) {
			pulsed = true;
			last = !last;
			timeout = now + REF_STALL_THRESHOLD;
		}
		if (motor_run(now, pulsed))
			break;
	}
	if (motor_error) {
		DISABLE_ENCODER();
		__asm__("halt");
		return;
	}
	
	lcd_sync();
	lcd_set_digit(LCD_DEG_3, 1);

	// turn forward
	motor_dir = FORWARD;
	motor_position = 0;
	now = get_tick();
	timeout = now + MAX_STALL_THRESHOLD;
	timeout2 = now + 30000;
	timeouts = 0; // timeout can't go beyond/near 65535/2. so use this counter
	mean_start = now;
	mean_count = MEAN_COUNT;
	lowest = UINT16_MAX;
	state = searching_start;
	while (true) {
		bool pulsed = false;
		now = get_tick();
		if (tick_elapsed(now, timeout)) {
			motor_stop = true;
			motor_error = 2;
			lcd_set_digit(LCD_TIME_1, 0xA);
		}
		if (tick_elapsed(now, timeout2)) { // overall timeout
			if (timeouts++ == 0)
				timeout2 += 30000;
			else {
				motor_stop = true;
				lcd_set_digit(LCD_TIME_1, 0xB);
				motor_error = 3;
			}
		}
		if ((bool)(MOTOR_ENC_PORT_IDR & MOTOR_ENC) != last) {
			pulsed = true;
			last = !last;
			timeout = now + MAX_STALL_THRESHOLD;
			if (!last) { // only use falling edges (when sensor is on reflective part of counter wheel)
				uint16_t diff;
				diff = now - last_t;
				mem[mem_in++] = diff & 0xff;
				motor_position++;
				if (--mean_count == 0) {
					diff = now - mean_start;
					mean_start = now;
					mean_count = MEAN_COUNT;
					if (state == searching_start) {
						if (diff < lowest)
							lowest = diff;
						if (diff < lowest + VALVE_START_THRESHOLD)
							motor_position = 0;
						else {
							lcd_set_digit(LCD_TIME_1, 0x1);
							state = searching_flat;
							mean2_start = now;
							mean2_count = MEAN2_COUNT;
							mean2 = 0;
						}
					} else {
						if (state == searching_flat) {
							if (mean2 == 0 || diff - mean2 >= MEAN2_THRESHOLD)
								state_counter = 0;
							else if (++state_counter == MEAN2_HOLD) {
								lcd_set_digit(LCD_TIME_1, 0x2);
								state = searching_end;
								state_counter = 0;
							}
						} else { // searching_end
							if (diff - mean2 < MEAN2_THRESHOLD)
								state_counter = 0;
							else if (++state_counter == MEAN2_HOLD) { // we found the end
								lcd_set_digit(LCD_TIME_1, 0x3);
								motor_stop = true;
							}
						}

						if (--mean2_count == 0) {
							mean2 = (now - mean2_start) >> MEAN2_SHIFT;
							mean2_start = now;
							mean2_count = MEAN2_COUNT;
						}
					}
				}
			}
		}
		if ((PF_IDR & BUTTON_LEFT) == 0)
			motor_stop = true;
		if (motor_run(now, pulsed))
			break;
	}
	motor_max_pos = motor_position;
	motor_percent = 0;

	lcd_sync();
	lcd_set_digit(LCD_DEG_3, 2);

#if 0
	{
		motor_position_t tmp = motor_position;
		lcd_set_digit(LCD_TIME_4, tmp % 10);
		tmp /= 10;
		lcd_set_digit(LCD_TIME_3, tmp % 10);
		tmp /= 10;
		lcd_set_digit(LCD_TIME_2, tmp % 10);
		tmp /= 10;
		lcd_set_digit(LCD_TIME_1, tmp % 10);
		tmp /= 10;
	}
#endif

	DISABLE_ENCODER();

	delay_ms(1000);
}

// move motor to have a valve opening of percent percent
void motor_move_to(uint8_t percent)
{
	uint16_t timeout;
	uint16_t now;
	motor_position_t threshold;
	uint8_t start_count;
	motor_position_t pos;
	bool last;

	if (motor_error)
		return;
	if (percent == 0 && motor_percent == percent)
		return;

	ENABLE_ENCODER();
	motor_percent = percent;
	ui_update();

#ifdef REF_ON_MOVE_ZERO
	if (percent == 0) { // move until stop
		threshold = 2 * MAX_STALL_THRESHOLD;
		pos = INT16_MAX;
	} else
#endif
	{
		// a * b / c
		// a * (b1 * 256 + b2) / c
		// (a * b1 * 256 + a * b2) / c
		// a * b1 / c * 256 + a * b2 / c
		uint16_t high = (uint16_t)percent * (motor_max_pos >> 8);
		uint16_t low = (uint16_t)percent * (motor_max_pos & 0xff);
		high += low >> 8;
		low &= 0xff;
		pos = high / 100;
		high -= pos * 100;
		pos <<= 8;
		low += high << 8;
		pos += (low + 50) / 100;
		pos = motor_max_pos - pos;

		//pos = motor_max_pos - ((uint32_t)percent * motor_max_pos + 50) / 100;
		threshold = 2 * MOVE_STALL_THRESHOLD;
	}

	if (pos == motor_position) {
		DISABLE_ENCODER();
		return;
	}
	if (pos > motor_position)
		motor_dir = FORWARD;
	else
		motor_dir = BACKWARD;

	start_count = START_PULSES - 1;
	now = get_tick();
	last = (bool)(MOTOR_ENC_PORT_IDR & MOTOR_ENC);
	timeout = now + threshold + 2 * BREAK_FREE_MARGIN;
	while (true) {
		bool pulsed = false;
		now = get_tick();
		if (tick_elapsed(now, timeout)) {
			motor_stop = true;
#ifdef REF_ON_MOVE_ZERO
			if (percent != 0) // only error if we were not moving to 0% (on block)
#endif
				motor_error = true;
		}
		if ((bool)(MOTOR_ENC_PORT_IDR & MOTOR_ENC) != last) {
			pulsed = true;
			last = !last;
			if (!last) { // only use falling edges (when sensor is on reflective part of counter wheel)
				motor_position += motor_dir;
				timeout = now + threshold;
				if (start_count) {
					start_count--;
					timeout += 2 * BREAK_FREE_MARGIN;
				}
				if (motor_position == pos)
					motor_stop = true;
			}
		}
		if ((PF_IDR & BUTTON_LEFT) == 0)
			motor_stop = true;
		if (motor_run(now, pulsed))
			break;
	}
#ifdef REF_ON_MOVE_ZERO
	if (percent == 0) { // remember newly found max_pos
		motor_max_pos = motor_position;
		// TODO check for too low motor max position and generate error
	}
#endif

	DISABLE_ENCODER();
}

#define OFF 0
#define LOW 1
#define HIGH 2
static void set_motor(int8_t left, int8_t right)
{
	uint8_t odr = PD_ODR & ~MOTORS_MASK;

	if (left == OFF)
		odr |= MOTOR_LEFT_HIGH;
	else if (left == LOW)
		odr |= MOTOR_LEFT_LOW | MOTOR_LEFT_HIGH;
	//else if (left == HIGH)
	//	odr |= 0;
	
	if (right == OFF)
		odr |= MOTOR_RIGHT_HIGH;
	else if (right == LOW)
		odr |= MOTOR_RIGHT_LOW | MOTOR_RIGHT_HIGH;
	//else if (left == HIGH)
	//	odr |= 0;

	PD_ODR = odr;
}

static void setup_timer()
{
	uint16_t duty;

	CLK_PCKENR1 |= CLK_PCKENR1_TIM1;
	TIM1_CR1 = TIM_CR1_DIR; // down-counter, not yet enabled
	TIM1_CNTRH = 0; // set counter to zero, so when enabling pwm mode the output will be active
	TIM1_CNTRL = 0
	// TODO do we need UG to force an update of all registers? i think not because we don't preload any register...
	duty = 800 - ((battery_voltage - 20) * 19 + (battery_voltage - 20 + 1) >> 1); // see motor-duty.ods. values captured of eq3 firmware
	if (motor_dir == FORWARD) {
		TIM1_CCR2H = duty >> 8;
		TIM1_CCR2L = duty & 0xff;
		TIM1_CCMR2 = (0b110 << TIM_CCMR_OCxM); // channel is output in pwm mode 1
		TIM1_CCER1 = TIM_CCER1_CC2P | TIM_CCER1_CC2E;
	}
	else {
		TIM1_CCR3H = duty >> 8;
		TIM1_CCR3L = duty & 0xff;
		TIM1_CCMR3 = (0b110 << TIM_CCMR_OCxM); // channel is output in pwm mode 1
		TIM1_CCER2 = TIM_CCER2_CC3P | TIM_CCER2_CC3E;
	}
	duty += 20; // set CCR1 to a 2% higher duty, so we can safely stop the timer before the output is enabled for the next pulse
	TIM1_CCR1H = duty >> 8;
	TIM1_CCR1L = duty & 0xff;
	TIM1_ARRH = 0x3; // 0x3e7 = 999 = 2ms period
	TIM1_ARRL = 0xe7;
}

static void disable_timer()
{
	TIM1_CR1 = 0; // disable counter
	TIM1_CCER1 = 0; // disable output compare
	TIM1_CCER2 = 0; // disable output compare
	TIM1_CCMR2 = 0; // set output mode to frozen. this way the change to pwm mode 1
	TIM1_CCMR3 = 0; // for the next move will update the output immediately
	// TODO is this true/neccessary even in pwm mode? or is OC1REF always/asynchronously updated?
	CLK_PCKENR1 &= ~CLK_PCKENR1_TIM1;
}


uint16_t motor_timeout;
uint16_t motor_steady_timeout;
bool timer_output_active;
enum { Idle, TurnSteady, TurnPWM, CrossOver, Short, Stop } motor_state = Idle;
bool motor_run(uint16_t tick, bool pulsed)
{
	// don't check tick for this state because it may have overflowed (several times)!
	if (motor_state == Idle) {
		motor_state = TurnSteady;
		motor_timeout = tick + 5; // run for at least 5ms
		motor_steady_timeout = tick + 200; // run with full power for 200ms
		setup_timer(); // this already enables the high side transistor!
		if (motor_dir == FORWARD)
			set_motor(OFF, LOW); // don't set one output high here, this is handled by the timer.
		else
			set_motor(LOW, OFF); // when disabling the timer, the high side transistor should be immediately off

		return false;
	}

	if (!tick_elapsed(tick, motor_timeout)) {
		if (motor_state == Short && pulsed) // stay in Short until there are no more pulses for at least 50ms
			motor_timeout = now + 50;
		return false;
	}
	
	if (motor_state == TurnSteady) {
		if (tick_elapsed(tick, motor_steady_timeout) || motor_stop) {
			motor_state = TurnPWM;
			TIM1_CR1 |= TIM_CR1_CEN; // enable timer
			timer_output_active = true; // it was true, but when this variable is set the timer has overflowed and so the output is inactive
		} else
			return false;
	}
	if (motor_state == TurnPWM) {
		uint8_t flags = TIM1_SR1; // from here to disable_timer() below should elapse no more than 20 clocks! TODO
		if (flags & TIM_SR1_UIF) // first check UIF because we don't clear the flags before starting the timer. so for the first time both flags may be set but the UIF flag is the most recently set one
			timer_output_active = false;
		else if (flags & TIM_SR1_CC1F)
			timer_output_active = true;
		TIM1_SR1 = ~flags; // clear all (read) flags

		if (motor_stop && !timer_output_active) {
			disable_timer();
			motor_state = CrossOver;
			motor_timeout = tick + 1;
		} else
			motor_timeout = tick; // to not let it overflow
	}
	else if (motor_state == CrossOver) {
		motor_state = Short;
		set_motor(LOW, LOW);
		motor_timeout = tick + 50;
	}
	else if (motor_state == Short) {
		motor_state = Stop;
		set_motor(OFF, OFF);
		motor_timeout = tick + 10; // 10ms before turning on again
	}
	else if (motor_state == Stop) {
		motor_state = Idle;
		motor_stop = false;

		return true; // movement finished
	}

	return false;
}
