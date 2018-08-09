
uint16_t motor_pos;
bool motor_dir = false;
bool motor_stop = false;

bool motor_run();

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

uint16_t ref_dir = false;
uint16_t time_between_enc = 0;
void motor_ref()
{
	uint16_t timeout;
	bool last;
	ENABLE_ENCODER();

	// turn backward until we don't see an encoder impuls for longer than x
	motor_dir = ref_dir;
	timeout = get_tick() + 200;
	last = false;
	while (true) {
		if (tick_elapsed(timeout))
			motor_stop = true;
		if ((bool)(MOTOR_ENC_PORT_IDR & MOTOR_ENC) != last) {
			last = !last;
			time_between_enc = get_tick() - timeout + 200;
			timeout = get_tick() + 200;
		}
		if (motor_run())
			break;
	}

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
bool motor_run()
{
	uint16_t tick = get_tick();
	if (!tick_elapsed(tick, motor_timeout))
		return false;
	
	if (motor_state == Stop) {
		motor_state = Turn;
		motor_timeout += 5 * 1024;
		if (motor_dir)
			set_motor(HIGH, LOW);
		else
			set_motor(LOW, HIGH);
	}
	else if (motor_state == Turn) {
		if (motor_stop) {
			motor_stop = false;
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
		motor_state = Stop;
		set_motor(OFF, OFF);
		motor_timeout = tick + 10; // 10ms before turning on again

		lcd_data.value = motor_state;
		return true; // movement finished
	}
	lcd_data.value = motor_state;

	return false;
}
