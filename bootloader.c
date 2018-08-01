#include <stdint.h>
#include <stdbool.h>

#include "stm8l.h"
#include "radio.h"

#include "time.c"

#include "si4430.c"


//#define USE_LSE

#define MSG_RESPONSE_ACK         0x00
#define MSG_RESPONSE_NACK        0x80

void send_response(as_packet_t * recvd, bool ack);
bool receive_cb();

const uint8_t hm_id[] = { 0x9a, 0x45, 0xa0 };
const uint8_t hm_serial[] = { 'M','Y','H','M','C','C','R','T','D','N' };

#define CMP_ID(a, b) (((a)[0] == (b)[0] && (a)[1] == (b)[1] && (a)[2] == (b)[2]) ? 0 : 1)
#define LIST_ID(a) (a)[0], (a)[1], (a)[2]

#include "lcd.c"

void main()
{
	// low speed external clock prevents debugger from working :(
#ifdef USE_LSE
	CLK_SWCR |= CLK_SWCR_SWEN;
	CLK_SWR = CLK_SWR_LSE; // LSE oscillator

	while (CLK_SWCR & CLK_SWCR_SWBSY)
		;

	// clock divier to 1
	CLK_CKDIVR = 0x0;
	
	// stop HSI
	CLK_ICKCR &= CLK_ICKCR_HSION;

	// f_sysclk = 32768Hz
#else
	// clock divider to 128
	//CLK_CKDIVR = 0b111; // TODO for test only
	// enable low speed external manually (it's needed for RTC clk which is needed for LCD clk)
	CLK_ECKCR |= CLK_ECKCR_LSEON;
	while (!(CLK_ECKCR & CLK_ECKCR_LSERDY))
		;
	// f_sysclk = 16MHz / 128 = 125kHz
#endif

#if 0 // TODO not needed?
	// configure RTC clock
	CLK_CRTCR = (0b1000<<CLK_CRTCR_RTCSEL); // use LSE as RTC clk source
	while (CLK_CRTCR & CLK_CRTCR_RTCSWBSY) // wait for the switch to finish
		;
#endif

	tick_init();
	radio_init();
	lcd_init();

#if 1
	// send hello packet
	{
		as_packet_t packet = { .data = {
			0x14, 0x00, 0x00, 0x10, LIST_ID(hm_id), 0x00, 0x00, 0x00, 0x00,
			hm_serial[0], hm_serial[1], hm_serial[2], hm_serial[3], hm_serial[4],
			hm_serial[5], hm_serial[6], hm_serial[7], hm_serial[8], hm_serial[9]
		}};
		radio_send(&packet);
		if (!radio_wait(get_tick() + 5000))
			__asm__("break");
	}
#endif

	lcd_sync();
	lcd_set_digit(read_int() ? LCD_DEG_1 : LCD_DEG_2, 0);
	lcd_set_digit(LCD_DEG_3, stati_in);

	{
		bool a = radio_wait(get_tick() + 5000);

		lcd_sync();
		lcd_set_digit(read_int() ? LCD_DEG_1 : LCD_DEG_2, 1);
	}

	radio_wait(get_tick() + 5000);

	lcd_sync();
	lcd_set_digit(LCD_DEG_1, 2);
	if (!receive_cb())
		__asm__("break"); // start main here

	lcd_sync();
	lcd_set_digit(LCD_DEG_1, 3);

	// switch to 100k mode
	radio_switch_100k();

	lcd_sync();
	lcd_set_digit(LCD_DEG_1, 4);

	if (!receive_cb())
		__asm__("break"); // start main here

	lcd_sync();
	lcd_set_digit(LCD_DEG_1, 0xF);

	while (true)
		;




}

void send_response(as_packet_t * recvd, bool ack)
{
	as_packet_t answer = { .length = 10, .counter = recvd->counter, .flags = 0x80, .type = 0x02, .from = { LIST_ID(hm_id) }, .to = { LIST_ID(recvd->from) }, .payload = { ack ? MSG_RESPONSE_ACK : MSG_RESPONSE_NACK } };

	if (ack && !(recvd->flags & 0x20)) // no ACK required
		return;

	radio_send(&answer);
	if (!radio_wait(1000))
		__asm__("break");
}
	
bool receive_cb()
{
	uint16_t timeout = get_tick() + (uint16_t)10000u;
	while (true) {
		//radio_enter_receive(15);
		if (!radio_wait(timeout))
			return false;
	
		if (!radio_receive(&packet, 15))
			continue; // if crc check fails, continue receiving

		if (CMP_ID(packet.to, hm_id) != 0)
			continue; // not for us

		if (packet.type != 0xCB) {
			send_response(&packet, false); // nack
			continue; // no "call bootloader" cmd
		}

		send_response(&packet, true);
		return true;
	}
}



