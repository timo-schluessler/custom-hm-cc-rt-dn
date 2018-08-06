#include <stdint.h>
#include <stdbool.h>

#define MAX_PAYLOAD 37

#include "stm8l.h"
#include "radio.h"

#include "time.c"
 
#include "si4430.c"

void debug(uint16_t nr);

//#define USE_LSE

#define MSG_RESPONSE_ACK         0x00
#define MSG_RESPONSE_NACK        0x80

void send_response(as_packet_t * recvd, bool ack);
bool receive_cb();
bool receive_block(uint16_t timeout_at, uint8_t expected_counter);
void start_main();
void (*write_flash_block)(uint16_t addr);
void write_flash_block_flash(uint16_t addr);
void copy_function_to_ram();

const uint8_t hm_id[] = { 0x9a, 0x45, 0xa0 };
const uint8_t hm_serial[] = { 'M','Y','H','M','C','C','R','T','D','N' };

#define CMP_ID(a, b) (((a)[0] == (b)[0] && (a)[1] == (b)[1] && (a)[2] == (b)[2]) ? 0 : 1)
#define LIST_ID(a) (a)[0], (a)[1], (a)[2]

#include "lcd.c"

#define BLOCK_COUNT (512 - 32) // 64kb - 8kb (bootloader) = 128 * (512 - 16)
#define BLOCKSIZE 128
#define MAIN_START (0x8000 + 0x2000)
uint8_t buf[BLOCKSIZE];

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
	CLK_CKDIVR = 0;
	// enable low speed external manually (it's needed for RTC clk which is needed for LCD clk)
	CLK_ECKCR |= CLK_ECKCR_LSEON;
	while (!(CLK_ECKCR & CLK_ECKCR_LSERDY))
		;
	// f_sysclk = 16MHz / 128 = 125kHz
#endif

#if 1 // TODO not needed?
	// configure RTC clock
	CLK_CRTCR = (0b1000<<CLK_CRTCR_RTCSEL); // use LSE as RTC clk source
	while (CLK_CRTCR & CLK_CRTCR_RTCSWBSY) // wait for the switch to finish
		;
#endif

	tick_init();
	radio_init();
	lcd_init();

	copy_function_to_ram();

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
	lcd_set_digit(LCD_DEG_1, 0);

	if (!receive_cb())
		start_main();

	lcd_sync();
	lcd_set_digit(LCD_DEG_1, 1);

	// switch to 100k mode
	radio_switch_100k();

	lcd_sync();
	lcd_set_digit(LCD_DEG_1, 2);

	if (!receive_cb())
		start_main();

	{
		uint16_t block = 0;
		uint8_t expected_counter = packet.counter + 1;
		uint16_t timeout = get_tick() + 10000;
		
		while (true) {
			if (tick_elapsed(timeout))
				start_main();

			if (!receive_block(timeout, expected_counter))
				continue;

			lcd_sync();
			lcd_set_digit(LCD_DEG_1, (3 + block) & 0xf);

			(*write_flash_block)((uint16_t)block * BLOCKSIZE);

			send_response(&packet, true);
			block++;
			expected_counter++;
			timeout = get_tick() + 10000;

			if (block == BLOCK_COUNT)
				break;
		}
	}

	

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
	if (!radio_wait(get_tick() + 1000))
		__asm__("break");
}
	
bool receive_cb()
{
	uint16_t timeout = get_tick() + 10000;
	while (!tick_elapsed(timeout)) {
		radio_enter_receive(15);
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
	return false;
}

#if 1
#define DEBUG(x, y) do { \
	lcd_sync(); \
	lcd_set_digit(x, y); \
} while (false);
#else
#define DEBUG(x, y) do { \
} while (false);
#endif

bool receive_block(uint16_t timeout_at, uint8_t expected_counter)
{
	bool first = true;
	uint16_t byte = 0;
	uint16_t block_size = 0;

	radio_enter_receive(AS_HEADER_SIZE + MAX_PAYLOAD);
	while (!tick_elapsed(timeout_at)) {
		if (!radio_wait(timeout_at)) {
			DEBUG(LCD_DEG_3, 0xf);
			return false;
		}
		if (!radio_receive(&packet, AS_HEADER_SIZE + MAX_PAYLOAD)) {
			DEBUG(LCD_DEG_3, 0);
			radio_enter_receive(AS_HEADER_SIZE + MAX_PAYLOAD);
			continue; // crc failed -> continue receiving
		}	
		radio_enter_receive(AS_HEADER_SIZE + MAX_PAYLOAD);
		if (CMP_ID(packet.to, hm_id) != 0) {
			DEBUG(LCD_DEG_3, 1);
			continue; // not for us
		}
		if (packet.type != 0xca) {
			DEBUG(LCD_DEG_3, 2);
			continue; // wrong packet type
		}

		if (packet.counter != expected_counter) {
			DEBUG(LCD_DEG_3, 3);
			radio_wait(get_tick()); // cancel receiving (wait with timeout = 0)
			return false;
		}

		if (first) {
			uint8_t length = packet.length - AS_HEADER_SIZE - 2;
			block_size = ((uint16_t)packet.payload[0] << 8) + packet.payload[1];
			if (block_size > BLOCKSIZE) {
				DEBUG(LCD_DEG_3, 4);
				radio_wait(get_tick()); // cancel receiving (wait with timeout = 0)
				return false;
			}


			memcpy(buf, packet.payload + 2, length);
			byte += length;

			first = false;
		} else {
			uint8_t length = packet.length - AS_HEADER_SIZE;
			if (byte + length > BLOCKSIZE) {
				DEBUG(LCD_DEG_3, 5);
				radio_wait(get_tick()); // cancel receiving (wait with timeout = 0)
				return false;
			}

			memcpy(buf + byte, packet.payload, length);
			byte += length;
		}

		//debug(byte);

		if (byte == block_size && packet.flags == 0x20) { // page received completly and ack requested
			radio_wait(get_tick()); // cancel receiving (wait with timeout = 0)
			return true;
		}
	}

	DEBUG(LCD_DEG_3, 6);
	return false;
}

void debug(uint16_t nr)
{
	lcd_sync();
	lcd_set_digit(LCD_TIME_4, (nr >>  0) & 0xf);
	lcd_set_digit(LCD_TIME_3, (nr >>  4) & 0xf);
	lcd_set_digit(LCD_TIME_2, (nr >>  8) & 0xf);
	lcd_set_digit(LCD_TIME_1, (nr >> 12) & 0xf);
}

void start_main()
{
	__asm
	jp [MAIN_START + 2]
	__endasm;
}

void write_flash_block_flash(uint16_t off)
{
	(void*)off;
	__asm
	
	// unlock flash programming
	mov	0x5052, #0x56 // FLASH_PUKR
	mov	0x5052, #0xae // FLASH_PUKR
00001$:
	ld		a, 0x5054 // FLASH_IAPSR
	bcp	a, #0x02 // PUL
	jreq	00001$

	// enable block programming
	mov	0x5051, #0x01 // FLASH_CR2 <- FLASH_CR2_PRG

	ldw	x, #_buf // source
	ldw	y, (0x3, sp) // destination offset

	// copy 128 (0x80) bytes
	ld		a, #0x80
	ld		(0x00, sp), a
00002$:
	ld		a, (x)
	ldf	(MAIN_START, y), a // use ldf because MAIN_START + 64kb is > 0x10000
	incw	x
	incw	y
	dec	(0x00, sp)
	jrne	00002$

	// wait for completion
00003$:
	ld		a, 0x5054 // FLASH_IAPSR
	bcp	a, #0x04 // EOP
	jreq	00003$

	// lock flash programming
	//bres	0x5054, #1

	__endasm;


#if 0
{
	uint8_t * addr = 0xa0000;
	uint8_t * in = buf;

	// disable flash memory write protection
	FLASH_PUKR = 0x56;
	FLASH_PUKR = 0xAE;
	while (!(FLASH_IAPSR & FLASH_IAPSR_PUL))
		;

	FLASH_CR2 = FLASH_CR2_PRG;
	for (uint8_t i = 128; i != 0; --i)
		*addr++ = *in++;
	
	while (!(FLASH_IAPSR & FLASH_IAPSR_EOP)) // busy wait for end of programming
		;

	FLASH_IAPSR &= ~FLASH_IAPSR_PUL; // reenable lock
}
#endif
}

uint8_t ram_function_space[128]; // last time i checked the function it took 68 byte. so we have some spare bytes
void copy_function_to_ram()
{
	for (uint8_t i = 0; i < sizeof(ram_function_space); i++)
		ram_function_space[i] = ((uint8_t*)&write_flash_block_flash)[i];

	write_flash_block = (void (*)(uint16_t))ram_function_space;
}
