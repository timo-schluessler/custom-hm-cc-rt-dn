
uint16_t motor_position;
bool motor_dir = false;
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

#define REF_STALL_THRESHOLD 100
#define MAX_STALL_THRESHOLD 100

uint16_t time_between_enc = 0;
uint16_t motor_max_pos;
bool motor_error;

uint16_t next_debug;
uint8_t mem[1400];
uint16_t mem_in = 0;

void motor_ref()
{
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
	motor_dir = false;
	now = get_tick();
	timeout = now + REF_STALL_THRESHOLD;
	timeout2 = now + 30000;
	timeouts = 0;
	last = false;
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
			time_between_enc = now - timeout + REF_STALL_THRESHOLD;
			timeout = now + REF_STALL_THRESHOLD;
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
	// use first fixed impulses to determine START position?
	motor_dir = true;
	motor_position = 0;
	next_debug = 8;
	now = get_tick();
	time_between_enc = now;
	timeout = now + 2 * MAX_STALL_THRESHOLD;
	timeout2 = now + 30000;
	timeouts = 0; // timeout can't go beyond/near 65535/2. so use this counter
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
			if (last) { // only use rising edges
				uint16_t diff = now - timeout + 2 * MAX_STALL_THRESHOLD;
				mem[mem_in++] = diff & 0xff;
				motor_position++;
				timeout = now + 2 * MAX_STALL_THRESHOLD;
				/*if (motor_position == next_debug) {
					uint16_t diff = now - time_between_enc;
					lcd_sync();
					lcd_set_digit(LCD_DEG_1, (diff >> 8) & 0xf);
					lcd_set_digit(LCD_DEG_2, (diff >> 4) & 0xf);
					next_debug += 16;
					time_between_enc = now;
				}*/
			}
		}
		if ((PF_IDR & BUTTON_LEFT) == 0)
			motor_stop = true;
		if (motor_run(now))
			break;
	}

	lcd_sync();
	lcd_set_digit(LCD_DEG_3, 2);

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

uint16_t motor_timeout = 1024;
enum { Stop, Turn, CrossOver, Recirculate } motor_state = Stop;
bool motor_run(uint16_t tick)
{
	if (!tick_elapsed(tick, motor_timeout))
		return false;
	
	if (motor_state == Stop) {
		motor_state = Turn;
		motor_timeout = tick + 50; // run for at least 50ms
		if (motor_dir)
			set_motor(HIGH, LOW);
		else
			set_motor(LOW, HIGH);
	}
	else if (motor_state == Turn) {
		if (motor_stop) {
			motor_state = CrossOver;
			motor_timeout = tick + 20;
			if (motor_dir)
				set_motor(OFF, LOW);
			else
				set_motor(LOW, OFF);
		} else
			motor_timeout = tick; // to not let it overflow
	}
	else if (motor_state == CrossOver) {
		set_motor(LOW, LOW);
		motor_state = Recirculate;
		motor_timeout = tick + 500;
	}
	else if (motor_state == Recirculate) {
		motor_stop = false;
		motor_state = Stop;
		set_motor(OFF, OFF);
		motor_timeout = tick + 10; // 10ms before turning on again

		return true; // movement finished
	}

	return false;
}
