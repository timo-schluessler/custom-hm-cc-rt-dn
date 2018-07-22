#include "radio.h"

#include "delay.h"
//#include "cc1101.h"
#include "si4430.h"

#include "spi.h"
#include "spi.c"

#include <stdint.h>
#include <stdbool.h>
#include <string.h>

#include "stm8l.h"

#define read_int() ((bool)(PB_IDR & nIRQ))

//static void strobe(uint8_t cmd);
static uint8_t read_reg(uint8_t addr);
static void read_burst(uint8_t addr, uint8_t len, uint8_t * data);

static void write_reg(uint8_t addr, uint8_t data);
static void write_burst(uint8_t addr, uint8_t len, uint8_t * data);

static uint8_t status;

#include "as.c"

#ifdef DEBUG_STATI
typedef struct {
	uint8_t status1;
	uint8_t status2;
} status_t;
status_t stati[50] = { { 0 } };
uint8_t stati_in = 0;
#endif

void radio_init()
{
	const uint8_t reg_1d_25[] = { 0x3C, 0x02, 0x03, 0x90, 0x20, 0x51, 0xDC, 0x00, 0x58 };
	const uint8_t reg_32_39[] = { 0x00, 0x0e, 0x08, 0x22, 0xE9, 0xCA, 0xE9, 0xCA };
	const uint8_t reg_6e_77[] = { 0x51, 0xDC, 0x2c, 0x22, 0x1E, 0x00, 0x00, 0x73, 0x67, 0x9F };

	spi_init();
	spi_enable();

	// reset CC1101
	_delay_us(5);
	select();
	_delay_us(10);
	deselect();
	_delay_us(41);

	//strobe(CC1101_SRES);

	partnum = read_reg(SI4430_DEVICE_TYPE);
	version = read_reg(SI4430_DEVICE_VERSION);

	// reset and wait for chip ready
	write_reg(SI4430_OMFC1, SI4430_SWRES | SI4430_XTON);
	while (!(read_reg(SI4430_STATUS2) & SI4430_CHIPRDY))
			;

	write_reg(SI4430_GPIOCONF2, 0x1f); // set GPIO2 to GND output (instead of default CLK output!)

	// write all settings from Si443x-Register-Settings.ods
	write_burst(0x1d, sizeof(reg_1d_25), reg_1d_25);
	write_reg(SI4430_RSSITH, 100);
	write_reg(0x2a, 0xff);

	write_reg(0x30, 0x89); // TODO disable crc check
	write_burst(0x32, sizeof(reg_32_39), reg_32_39);

	write_reg(0x3e, 0x3d);
	write_reg(0x58, 0x80);
	write_reg(0x69, 0x60);
	
	write_burst(0x6e, sizeof(reg_6e_77), reg_6e_77);

	write_reg(SI4430_INT1, SI4430_PKGVALID | SI4430_PKGSENT); // enable pkg valid/sent interrupt
	//write_reg(SI4430_INT2, SI4430_SWDET | SI4430_PREAVAL | SI4430_RSSI | SI4430_CHIPRDY); // enable sync word detect interrupt

	radio_enter_receive(14);
}

void radio_switch_100k()
{
	const uint8_t reg_1c_25[] = { 0x9A, 0x44, 0x0A, 0x03, 0x3C, 0x02, 0x22, 0x22, 0x07, 0xFF };
	const uint8_t reg_6e_72[] = { 0x19, 0x9A, 0x0C, 0x22, 0x4C };

	// write all changed settings from Si443x-Register-Settings-100k.ods
	write_burst(0x1c, sizeof(reg_1c_25), reg_1c_25);
	write_reg(0x2a, 0x48);
	write_reg(0x58, 0xC0);
	write_burst(0x6e, sizeof(reg_6e_72), reg_6e_72);
}

void white(uint8_t len, uint8_t * data)
{
	const uint8_t pn9[] = { 0xff, 0xe1, 0x1d, 0x9a, 0xed, 0x85, 0x33, 0x24, 0xea, 0x7a, 0xd2, 0x39, 0x70, 0x97, 0x57, 0x0a, 0x54, 0x7d, 0x2d };
	if (len > sizeof(pn9))
		__asm__ ("break");
	for (uint8_t i = len - 1; i != 0; i--)
		data[i] ^= pn9[i];
	data[0] ^= pn9[0];
}

// TODO move this to RAM to further reduce current consumption
static bool wait_int(uint32_t timeout)
{
	WFE_CR1 = WFE_CR1_TIM2_EV; // timer 2 
	WFE_CR2 = WFE_CR2_EVB_G | WFE_CR2_EXTI_EV4; // nIQR is PB4

	wfe();
}

static uint8_t read_status()
{
	if (!read_int()) {
		uint8_t status1 = read_reg(SI4430_STATUS1);
		uint8_t status2 = read_reg(SI4430_STATUS2);
#ifdef DEBUG_STATI
		stati[stati_in].status1 = status1;
		stati[stati_in].status2 = status2;
		if (++stati_in == sizeof(stati) / sizeof(status_t))
			stati_in = 0;
		lcd_data.value = stati_in & 0xf;
#endif
		return status1;
	}
	return 0;
}

void radio_enter_receive(uint8_t max_length)
{
	write_reg(SI4430_OMFC2, SI4430_FFCLRRX | SI4430_FFCLRTX); // clear fifos
	write_reg(SI4430_OMFC2, 0);
	write_reg(SI4430_PKLEN, max_length + 3);
	write_reg(SI4430_OMFC1, SI4430_RXON); // enter receive mode
	// TODO enable events for counter/timeout and nIRQ pin change -> then WFE
}

#include "crc16.c"
bool radio_received()
{
	return read_status() & SI4430_PKGVALID;
}

bool radio_receive(as_packet_t * pkg, uint8_t max_length)
{
	read_burst(SI4430_FIFO, max_length, pkg->data);
	white(max_length, pkg->data);
	if (pkg->length + 3 < max_length) // avoid accessing invalid memory (3 = 1 byte length + 2 byte crc)
		max_length = pkg->length + 3;
	if (crc16(0xffff, max_length, pkg->data) != 0)
		return false;
	decode(max_length - 3, pkg->data + 1);

	return true;
}

void radio_send(as_packet_t * pkg)
{
	uint16_t crc;

	encode(pkg->length, pkg->data + 1);
	crc = crc16(0xffff, pkg->length + 1, pkg->data);
	pkg->data[pkg->length + 1] = crc >> 8;
	pkg->data[pkg->length + 2] = crc & 0xff;
#if 1 // this is only to test this function! remove it once it worked once! TODO
	crc = crc16(0xffff, pkg->length + 3, pkg->data);
	if (crc != 0)
		__asm__ ("break");
#endif
	white(pkg->length + 3, pkg->data); // 1 byte length + 2 byte crc

	write_reg(SI4430_OMFC2, SI4430_FFCLRRX | SI4430_FFCLRTX); // clear fifos
	write_reg(SI4430_OMFC2, 0);
	write_reg(SI4430_PKLEN, pkg->length + 3);
	write_burst(SI4430_FIFO, pkg->length + 3, pkg->data);
	write_reg(SI4430_OMFC1, SI4430_TXON); // enter transmit mode

#if 0
	white(pkg_len, testpkg);
	crc = crc16(0xffff, pkg_len, testpkg);
	decode(pkg_len - 3, testpkg + 1);
#endif
}

bool radio_sent()
{
	return read_status() & SI4430_PKGSENT;
}


volatile as_packet_t packet;
volatile as_packet_t testpkg = { .data = { 14, 0x08, 0x90, 0x11, 0x2e, 0x16, 0x6e, 0x0F, 0x4D, 0x55, 0x02, 0x01, 0xC8, 0x00, 0x00 } };

void radio_poll()
{
	if (radio_received()) {
		if (!radio_receive(&packet, 14))
			__asm__ ("break");

		testpkg.counter = packet.counter; // answer with same seq-id
		radio_send(&testpkg);
	}
	else if (radio_sent())
		radio_enter_receive(14);
}

static void strobe(uint8_t cmd)
{
	select();
	wait_miso();
	status = spi_send_byte(cmd);
	deselect();
	_delay_us(1000);
}

static uint8_t read_reg(uint8_t addr)
{
	uint8_t val;

	select();
	//wait_miso(); // only cc1101
	spi_send_byte(addr | READ_BURST);
	val = spi_send_byte(0x0);
	deselect();

	return val;
}

static void read_burst(uint8_t addr, uint8_t len, uint8_t * data)
{
	select();
	spi_send_byte(addr | READ_BURST);
	for ( ; len; len--)
		*(data++) = spi_send_byte(0x0);
	deselect();
}

static void write_reg(uint8_t addr, uint8_t data)
{
	select();
	spi_send_byte(addr | WRITE_BURST);
	spi_send_byte(data);
	deselect();
}

static void write_burst(uint8_t addr, uint8_t len, uint8_t * data)
{
	select();
	spi_send_byte(addr | WRITE_BURST);
	for ( ; len; len--)
		spi_send_byte(*(data++));
	deselect();
}
