
#define WHEELS (WHEEL_A | WHEEL_B)
uint8_t last_wheel;
uint8_t wheel = 0;

void ui_init()
{
	last_wheel = PC_IDR & WHEELS;
}

bool in_ui_update = false;
bool need_ui_update = false;
void ui_update()
{
	// lock mutex using exchange command

}

// EXTI1
void wheel_a_isr() __interrupt(9)
{
	uint8_t cur_wheel;

	EXTI_SR1 = EXTI_SR1_P1F;
	cur_wheel = PC_IDR & WHEELS;

	if (cur_wheel == last_wheel)
		return;
	if ((cur_wheel ^ (cur_wheel >> 1)) & 1)
		wheel++;
	else
		wheel--;
	
	last_wheel = cur_wheel;

	ui_update();

	/* one direction
		01 -> 10
		10 -> 01
	*/
	/* the other
		00 -> 11
		11 -> 00
	*/
}

// EXTIF
void button_isr() __interrupt(5)
{
	EXTI_SR2 = EXTI_SR2_PFF;
}










#if 0
// this code nicely works to count on each edge. but for the encoder "ratches" only on each second count, we don't use this code
void wheeled()
{
	uint8_t wheel = PC_IDR & WHEELS;

	/* one direction
		00 -> 01 : 01 ^: 10
		01 -> 11 : 10 ^: 00
		11 -> 10 : 01 ^: 10
		10 -> 00 : 10 ^: 00
	*/
	/* the other
		00 -> 10 : 10 ^: 11
		10 -> 11 : 01 ^: 01
		11 -> 01 : 10 ^: 11
		01 -> 00 : 01 ^: 01
	*/
	
#if 0
	uint8_t change = wheel ^ last_wheel;
	uint8_t edge;
	if (change == 0)
		return;
	edge = (wheel >> (change - 1)); // rising or falling?
	if (((wheel >> (2 - change)) ^ edge ^ change) & 1)
		lcd_data.value++;
	else
		lcd_data.value--;
#else
	if (wheel == last_wheel)
		return;
	//if ((wheel ^ ((wheel << 1) | (wheel >> 1)) ^ change) & 1) // wheel << 1 has no effect because of the & 1
	//if ((wheel ^ (wheel >> 1) ^ change) & 1) // substitute change. and wheel ^ wheel = 0
	if ((last_wheel ^ (wheel >> 1)) & 1)
		lcd_data.value++;
	else
		lcd_data.value--;
#endif

	lcd_data.value &= 0xf;

	last_wheel = wheel;
}

// EXTI0
void wheel_b_isr() __interrupt(8)
{
	EXTI_SR1 = EXTI_SR1_P0F;
	wheeled();
}

// EXTI1
void wheel_a_isr() __interrupt(9)
{
	EXTI_SR1 = EXTI_SR1_P1F;
	wheeled();
}
#endif


