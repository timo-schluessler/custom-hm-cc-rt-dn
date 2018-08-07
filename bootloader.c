#include <stdint.h>
#include <stdbool.h>

#define F_CPU 16000000UL

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
bool receive_cb(uint16_t timeout_at);
bool receive_block(uint16_t timeout_at, uint8_t expected_counter);
void (*write_flash_block)(uint16_t addr);
void (*erase_unused_blocks)(uint16_t addr);
void copy_functions_to_ram();
bool app_crc_ok();
bool bootloader_buttons_pressed();

const uint8_t hm_id[] = { 0x9a, 0x45, 0xa0 };
const uint8_t hm_serial[] = { 'M','Y','H','M','C','C','R','T','D','N' };

#define CMP_ID(a, b) (((a)[0] == (b)[0] && (a)[1] == (b)[1] && (a)[2] == (b)[2]) ? 0 : 1)
#define LIST_ID(a) (a)[0], (a)[1], (a)[2]

#include "lcd.c"

#define BLOCK_COUNT (512 - 32) // 64kb - 8kb (bootloader) = 128 * (512 - 16)
#define BLOCKSIZE 128
#define FLASH_START 0x8000
#define MAIN_START (FLASH_START + 0x2000)
uint8_t buf[BLOCKSIZE];

static inline void start_main()
{
	__asm
	jp [MAIN_START + 2]
	__endasm;
}

static inline void restart()
{
	__asm
	jp [0x8000 + 2]
	__endasm;
}

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

#if 1 // TODO not needed? - only when using LCD
	// configure RTC clock
	CLK_CRTCR = (0b1000<<CLK_CRTCR_RTCSEL); // use LSE as RTC clk source
	while (CLK_CRTCR & CLK_CRTCR_RTCSWBSY) // wait for the switch to finish
		;
#endif

	if (app_crc_ok() && !bootloader_buttons_pressed())
		start_main();

	tick_init();
	radio_init();
	lcd_init();

	copy_functions_to_ram();

	// send hello packet
	{
		as_packet_t packet = { .data = {
			0x14, 0x00, 0x00, 0x10, LIST_ID(hm_id), 0x00, 0x00, 0x00, 0x00,
			hm_serial[0], hm_serial[1], hm_serial[2], hm_serial[3], hm_serial[4],
			hm_serial[5], hm_serial[6], hm_serial[7], hm_serial[8], hm_serial[9]
		}};
		radio_send(&packet);
		if (!radio_wait(get_tick() + 1000))
			restart();
	}

	lcd_sync();
	lcd_set_digit(LCD_DEG_1, 0);

	if (!receive_cb(get_tick() + 1500)) {
		if (app_crc_ok())
			start_main();
		else
			restart();
	}

	lcd_sync();
	lcd_set_digit(LCD_DEG_1, 1);

	// switch to 100k mode
	radio_switch_100k();

	lcd_sync();
	lcd_set_digit(LCD_DEG_1, 2);

	if (!receive_cb(get_tick() + 1000))
		restart();

	{
#define BLOCK_TIMEOUT 1500
		uint16_t block = 0;
		uint8_t expected_counter = packet.counter + 1;
		uint16_t timeout = get_tick() + BLOCK_TIMEOUT;
		
		while (true) {
			if (tick_elapsed(timeout))
				break;

			if (!receive_block(timeout, expected_counter))
				continue;

			lcd_sync();
			lcd_set_digit(LCD_DEG_1, (3 + block) & 0xf);

			(*write_flash_block)((uint16_t)block * BLOCKSIZE);

			send_response(&packet, true);
			block++;
			expected_counter++;
			timeout = get_tick() + BLOCK_TIMEOUT;

			if (block == BLOCK_COUNT)
				break;
		}

		// erase the remaining flash space
		// this is needed to find [ 0x00 crc ] when checking crc
		(*erase_unused_blocks)((uint16_t)block * BLOCKSIZE);
	}


	restart();

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
	
bool receive_cb(uint16_t timeout_at)
{
	while (!tick_elapsed(timeout_at)) {
		radio_enter_receive(15);
		if (!radio_wait(timeout_at))
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

void erase_unused_blocks_flash(uint16_t start_offset)
{
	(void*)start_offset;
	__asm

	// unlock flash programming
	mov	0x5052, #0x56 // FLASH_PUKR
	mov	0x5052, #0xae // FLASH_PUKR
00001$:
	ld		a, 0x5054 // FLASH_IAPSR
	bcp	a, #0x02 // PUL
	jreq	00001$

	// load start block address
	ldw	x, (0x3, sp)
	addw	x, #(MAIN_START - FLASH_START) // add this offset so we can easily check x == 0 below for end condition
	clrw	y

00002$:
	// enable block erasing
	mov	0x5051, #0x20 // FLASH_CR2 <- FLASH_CR2_ERASE

	ldw	(FLASH_START + 0, x), y
	ldw	(FLASH_START + 2, x), y

	// wait for completion
00003$:
	ld		a, 0x5054 // FLASH_IAPSR
	bcp	a, #0x04 // EOP
	jreq	00003$

	addw	x, #128 // increment one page. when x is zero again, we erased all blocks
	jrne	00002$

	// lock flash programming
	bres	0x5054, #1

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
	bres	0x5054, #1

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

uint8_t write_flash_block_ram[128]; // last time i checked the function it took 68 byte. so we have some spare bytes
uint8_t erase_unused_blocks_ram[128];
void copy_functions_to_ram()
{
	for (uint8_t i = 0; i < sizeof(write_flash_block_ram); i++)
		write_flash_block_ram[i] = ((uint8_t*)&write_flash_block_flash)[i];

	write_flash_block = (void (*)(uint16_t))write_flash_block_ram;

	for (uint8_t i = 0; i < sizeof(erase_unused_blocks_ram); i++)
		erase_unused_blocks_ram[i] = ((uint8_t*)&erase_unused_blocks_flash)[i];

	erase_unused_blocks = (void (*)(uint16_t))erase_unused_blocks_ram;
}

bool app_crc_ok()
{
	// search for flash block that ends with 0x55 crc crc. that is end of main program
	__asm
	ldw	x, #(0xfffd - (MAIN_START - FLASH_START)) // third last byte of block: equals 0x55 in last app block
	ld		a, #0x55

	// start at end of flash until MAIN_START
00001$:
	cp		a, (MAIN_START, x)
	jreq	00002$
	subw	x, #128
	jrnc	00001$ // if x didn't underflow, continue with next block

	// return false
	ld		a, #0
	ret

00002$:
	// calculate crc
	// stack: end offset[2], crc[2]
	sub	sp, #3

	addw	x, #3 // +3 to let app_end be the byte-offset after the last byte to be checked by crc
	ldw	(0x00, sp), x // store end offset
	clrw	y // cur offset
	ldw	x, #0xffff
	ldw	(0x02, sp), x // crc = 0xffff

00003$:
#if 0 // crc = crc16_tab[(crc >> 8) ^ (*buf++)] ^ (crc << 8); (version from crc16.c for homematic/cc1101 crc16)
	ld		a, (MAIN_START, y)
	incw	y // increment offset

	xor	a, (0x02, sp) // xor data with high byte crc
	clrw	x
	ld		xl, a
	sllw	x // time 2 (16 bit data table)
	ldw	x, (_crc16_tab, x) // load data table content

	ld		a, xh
	xor	a, (0x03, sp) // xor high byte table content with low byte crc
	ld		xh, a // write back to high byte crc, low byte stays as read from table
	ldw	(0x02, sp), x // safe crc
#else // crc = ((crc << 8) | *buf++) ^ crc16_tab[crc >> 8]; (version from srecord)
	ld		a, (0x02, sp) // high byte crc
	clrw	x
	ld		xl, a
	sllw	x // times 2 (16 bit data table)
	ldw	x, (_crc16_tab, x) // load data table content

	ld		a, xl
	xor	a, (MAIN_START, y) // data byte
	incw	y
	ld		xl, a

	ld		a, xh
	xor	a, (0x03, sp) // low byte crc
	ld		xh, a
	ldw	(0x02, sp), x // safe new crc
#endif

	ldw	x, y // load cur offset to x because cpw only works with x (when using sp offset)
	cpw	x, (0x0, sp)
	jrne	00003$

	// if crc is zero, return 1, else 0
	clr	a
	ldw	x, (0x02, sp) // load/test crc
	jrne	00004$
	inc	a

00004$:
	addw	sp, #3
	ret

	__endasm;
	return false;

#if 0

	// calculate crc
	sub	sp, #8
	ldw	x, #_crc16_tab
	ldw	(0x03, sp), x

00101$:
	// counter
	ld	a, (0x0d, sp)
	ld	xl, a
	dec	a
	ld	(0x0d, sp), a
	ld	a, xl
	tnz	a
	jreq	00103$

	ld	a, (0x0b, sp) // high byte crc
	ld	(0x08, sp), a

	ldw	x, (0x0e, sp) // data
	ld	a, (x)
	incw	x
	ldw	(0x0e, sp), x

	xor	a, (0x08, sp) // data xor high byte crc
	ld	xl, a

	// load table content
	clr	(0x05, sp)
	clr	(0x07, sp)
	ld	a, (0x05, sp)
	xor	a, (0x07, sp) // 0x0 xor 0x0 = 0x00 ???
	ld	xh, a // same as clr xh? or clr a; ld xh, a
	sllw	x // times 2 (16 bit table)
	addw	x, (0x03, sp) // add address of crc16_tab
	ldw	x, (x) // table content
	ldw	(0x01, sp), x

	ld	a, (0x0c, sp) // low byte crc
	ld	xh, a
	clr	a
	xor	a, (0x02, sp) // low byte table content
	rlwa	x // rotate left trough a by 1 byte
	xor	a, (0x01, sp) // high byte table content with low byte crc
	ld	xh, a
	ldw	(0x0b, sp), x // store as new crc
	jra	00101$
	ldw	x, (0x0b, sp)
	addw	sp, #8
	ret


	return true;
#endif
}

bool bootloader_buttons_pressed()
{
	PF_CR1 |= (1<<4) | (1<<6); // enable pullups
	delay_ms(100);
	return (PF_IDR & ((1<<4) | (1<<6))) == 0;
}








