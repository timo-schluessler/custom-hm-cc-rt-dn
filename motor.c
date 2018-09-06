
motor_position_t motor_position;
#define BACKWARD -1
#define FORWARD 1
int8_t motor_dir = BACKWARD;
bool motor_stop = false;

bool motor_run(uint16_t now);

#define MOTOR_LEFT_HIGH (1u<<4) // PD4
#define MOTOR_LEFT_LOW (1u<<6) // PD6
#define MOTOR_RIGHT_HIGH (1u<<5) // PD5
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

#define REF_STALL_THRESHOLD 25
#define MAX_STALL_THRESHOLD 25
#define MOVE_STALL_THRESHOLD 40
#define BREAK_FREE_MARGIN 100
#define START_PULSES 3 // the first rotation may take BREAK_FREE_MARGIN of additional time

#define MEAN_COUNT 12
#define VALVE_START_THRESHOLD 15

motor_position_t motor_max_pos;
bool motor_error;

//uint8_t mem[1400];
//uint16_t mem_in = 0;

void motor_ref()
{
	uint16_t lowest;
	uint16_t mean_start;
	uint8_t mean_count;
	uint8_t start_count;
	uint16_t now;
	uint16_t timeout;
	uint16_t timeout2;
	uint8_t timeouts;
	bool last;

	ENABLE_ENCODER();
	motor_error = false;

	lcd_sync();
	lcd_set_digit(LCD_DEG_3, 0);

	// turn backward until we don't see an encoder impuls for longer than x
	motor_dir = BACKWARD;
	start_count = START_PULSES - 1;
	now = get_tick();
	timeout = now + REF_STALL_THRESHOLD + BREAK_FREE_MARGIN;
	timeout2 = now + 30000;
	timeouts = 0;
	last = (bool)(MOTOR_ENC_PORT_IDR & MOTOR_ENC);
	while (true) {
		now = get_tick();
		if (tick_elapsed(now, timeout))
			motor_stop = true;
		if (tick_elapsed(now, timeout2)) { // overall timeout
			if (timeouts++ == 0)
				timeout2 += 30000;
			else {
				motor_stop = true;
				motor_error = true;
			}
		}
		if ((bool)(MOTOR_ENC_PORT_IDR & MOTOR_ENC) != last) {
			last = !last;
			timeout = now + REF_STALL_THRESHOLD;
			if (start_count) {
				start_count--;
				timeout += BREAK_FREE_MARGIN;
			}
		}
		if (motor_run(now))
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
	start_count = START_PULSES - 1;
	now = get_tick();
	timeout = now + 2 * MAX_STALL_THRESHOLD + 2 * BREAK_FREE_MARGIN;
	timeout2 = now + 30000;
	timeouts = 0; // timeout can't go beyond/near 65535/2. so use this counter
	mean_start = now;
	mean_count = MEAN_COUNT;
	lowest = UINT16_MAX;
	while (true) {
		now = get_tick();
		if (tick_elapsed(now, timeout))
			motor_stop = true;
		if (tick_elapsed(now, timeout2)) { // overall timeout
			if (timeouts++ == 0)
				timeout2 += 30000;
			else {
				motor_stop = true;
				motor_error = true;
			}
		}
		if ((bool)(MOTOR_ENC_PORT_IDR & MOTOR_ENC) != last) {
			last = !last;
			if (!last) { // only use falling edges (when sensor is on reflective part of counter wheel)
				uint16_t diff;
				//diff = now - timeout + 2 * MAX_STALL_THRESHOLD;
				//mem[mem_in++] = diff & 0xff;
				motor_position++;
				timeout = now + 2 * MAX_STALL_THRESHOLD;
				if (start_count) {
					start_count--;
					timeout += 2 * BREAK_FREE_MARGIN;
				}
				if (--mean_count == 0) {
					mean_count = MEAN_COUNT;
					diff = now - mean_start;
					mean_start = now;
					if (diff < lowest)
						lowest = diff;
					if (diff < lowest + VALVE_START_THRESHOLD)
						motor_position = 0;
					else if (motor_position == MEAN_COUNT)
						lcd_set_digit(LCD_TIME_1, 0xF);
				}
			}
		}
		if ((PF_IDR & BUTTON_LEFT) == 0)
			motor_stop = true;
		if (motor_run(now))
			break;
	}
	motor_max_pos = motor_position;

	lcd_sync();
	lcd_set_digit(LCD_DEG_3, 2);

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

	DISABLE_ENCODER();
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

	ENABLE_ENCODER();

	if (percent == 0) { // move until stop
		threshold = 2 * MAX_STALL_THRESHOLD;
		pos = INT16_MAX;
	} else {
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

	if (pos == motor_position)
		return;
	if (pos > motor_position)
		motor_dir = FORWARD;
	else
		motor_dir = BACKWARD;

	start_count = START_PULSES - 1;
	now = get_tick();
	last = (bool)(MOTOR_ENC_PORT_IDR & MOTOR_ENC);
	timeout = now + threshold + 2 * BREAK_FREE_MARGIN;
	while (true) {
		now = get_tick();
		if (tick_elapsed(now, timeout)) {
			motor_stop = true;
			if (percent != 0) // only error if we were not moving to 0% (on block)
				motor_error = true;
		}
		if ((bool)(MOTOR_ENC_PORT_IDR & MOTOR_ENC) != last) {
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
		if (motor_run(now))
			break;
	}
	if (percent == 0) { // remember newly found max_pos
		motor_max_pos = motor_position;
		// TODO check for too low motor max position and generate error
	}

	DISABLE_ENCODER();
}

#define OFF 0
#define LOW 1
#define HIGH 2
void set_motor(int8_t left, int8_t right)
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

uint16_t motor_timeout;
enum { Idle, Turn, CrossOver, Recirculate, Stop } motor_state = Idle;
bool motor_run(uint16_t tick)
{
	// don't check tick for this state because it may have overflowed (several times)!
	if (motor_state == Idle) {
		motor_state = Turn;
		motor_timeout = tick + 5; // run for at least 5ms
		if (motor_dir == FORWARD)
			set_motor(HIGH, LOW);
		else
			set_motor(LOW, HIGH);

		return false;
	}

	if (!tick_elapsed(tick, motor_timeout))
		return false;
	
	if (motor_state == Turn) {
		if (motor_stop) {
			motor_state = CrossOver;
			motor_timeout = tick + 5;
			if (motor_dir == FORWARD)
				set_motor(OFF, LOW);
			else
				set_motor(LOW, OFF);
		} else
			motor_timeout = tick; // to not let it overflow
	}
	else if (motor_state == CrossOver) {
		set_motor(LOW, LOW);
		motor_state = Recirculate;
		motor_timeout = tick + 20;
	}
	else if (motor_state == Recirculate) {
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