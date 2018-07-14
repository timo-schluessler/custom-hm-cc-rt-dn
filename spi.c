#include "spi.h"

#include "delay.h"
//#include "cc1101.h"
#include "si4430.h"

#include <stdint.h>
#include <stdbool.h>
#include <string.h>

#include "stm8l.h"

#define nSEL (1u<<3)
#define nIRQ (1u<<4)
#define MISO (1u<<7)
#define SCLK (1u<<5)
#define MOSI (1u<<6)
#define select() do { PB_ODR &= ~nSEL; } while(false)
#define deselect() do { PB_ODR |= nSEL; } while(false)
#define wait_miso() while (PB_IDR & MISO);

#define read_int() ((bool)(PB_IDR & nIRQ))

//static void strobe(uint8_t cmd);
static uint8_t read_reg(uint8_t addr);
static void read_burst(uint8_t addr, uint8_t len, uint8_t * data);

static void write_reg(uint8_t addr, uint8_t data);
static void write_burst(uint8_t addr, uint8_t len, uint8_t * data);

static uint8_t status;

extern volatile uint8_t partnum;
extern volatile uint8_t version;

volatile uint8_t status1 = 0xff;
volatile uint8_t status2 = 0xff;

#include "as.c"

typedef struct {
	uint8_t status1;
	uint8_t status2;
} status_t;
status_t stati[50] = { { 0 } };
uint8_t stati_in = 0;

void spi_init()
{
	const uint8_t reg_1d_25[] = { 0x3C, 0x02, 0x03, 0x90, 0x20, 0x51, 0xDC, 0x00, 0x58 };
	//const uint8_t reg_32_39[] = { 0x00, 0x06, 0x08, 0x22, 0xE9, 0xCA, 0xE9, 0xCA };
	const uint8_t reg_32_39[] = { 0x00, 0x0e, 0x08, 0x22, 0xE9, 0xCA, 0xE9, 0xCA }; // TODO fix pkglen
	const uint8_t reg_6e_77[] = { 0x51, 0xDC, 0x2c, 0x22, 0x1E, 0x00, 0x00, 0x73, 0x67, 0x9F };

	PB_ODR &= ~SCLK;
	PB_CR1 = nSEL | SCLK | MOSI; // push-pull
	PB_DDR = nSEL | SCLK | MOSI; // output
	deselect();

	CLK_PCKENR1 |= CLK_PCKENR1_SPI;
	SPI1_CR2 = SPI_CR2_SSM | SPI_CR2_SSI; // software slave management: master
	SPI1_CR1 = 0b0*SPI_CR1_BR | SPI_CR1_MSTR; // f_sysclk / 2, master

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

	//write_reg(SI4430_INT1, SI4430_PKGVALID | SI4430_PKGSENT); // enable pkg valid/sent interrupt
	write_reg(SI4430_INT1, SI4430_PKGVALID | SI4430_PKGSENT | 0xff); // enable pkg valid/sent interrupt
	write_reg(SI4430_INT2, SI4430_SWDET | SI4430_PREAVAL | SI4430_RSSI | SI4430_CHIPRDY); // enable sync word detect interrupt


	write_reg(SI4430_PKLEN, 14 + 3); // TODO
	write_reg(SI4430_RXAFTHR, 15); // TODO
	write_reg(SI4430_OMFC1, SI4430_RXON); // enter receive mode
	return;

#if 1
	write_reg(SI4430_OMFC1, SI4430_RXON); // enter receive mode
	while (1) {
		while (read_int())
			;
		status1 = read_reg(SI4430_STATUS1);
		status2 = read_reg(SI4430_STATUS2);
		stati[stati_in].status1 = status1;
		stati[stati_in].status2 = status2;
		if (++stati_in == sizeof(stati) / sizeof(status_t))
			stati_in = 0;
		if (read_reg(SI4430_STATUS1) & SI4430_PKGVALID)
			break;
	}
#endif

	{
		uint8_t test_data[] = { 0x31, 0xA0, 0x01, 0x0F, 0x4D, 0x55, 0x9A, 0x45, 0x9F, 0x00, 0x05, 0x00, 0x00, 0x00, 0x00, 0x00 };
		encode(sizeof(test_data), test_data);

		write_reg(SI4430_OMFC2, SI4430_FFCLRRX | SI4430_FFCLRTX); // clear fifos
		write_reg(SI4430_OMFC2, 0);
		write_reg(SI4430_PKLEN, sizeof(test_data)); // this must match fifo data length, otherwise the fifo will underflow
		write_burst(SI4430_FIFO, sizeof(test_data), test_data);
		write_reg(SI4430_OMFC1, SI4430_TXON); // enter transmit mode
		while (read_int()) // TODO use wfe? timeout?
			;
		status1 = read_reg(SI4430_STATUS1);
		status2 = read_reg(SI4430_STATUS2);
	}

	write_reg(SI4430_OMFC1, 0); // enter standby mode 
	spi_disable();

	if (partnum == 0 && version == 0 && status == 0)
		while (1)
			;
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

#include "crc16.c"

uint8_t pkg_len;
volatile uint8_t packet[64];
volatile uint8_t packet_decoded[64];
volatile uint8_t packet_unwhite[20];
volatile	uint16_t crc;

volatile uint8_t testpkg[] = { 0x0, 0x08, 0x90, 0x11, 0x2e, 0x16, 0x6e, 0x0F, 0x4D, 0x55, 0x02, 0x01, 0xC8, 0x00, 0x00, 0x00, 0x00 };
void spi_poll()
{
	if (!read_int()) {
		status1 = read_reg(SI4430_STATUS1);
		status2 = read_reg(SI4430_STATUS2);
		stati[stati_in].status1 = status1;
		stati[stati_in].status2 = status2;
		if (++stati_in == sizeof(stati) / sizeof(status_t))
			stati_in = 0;
		lcd_data.value = stati_in & 0xf;
		if (status1 & SI4430_PKGVALID) {
			pkg_len = read_reg(SI4430_PKLEN);
			//for (uint8_t i = 0; i < pkg_len; i++)
			//	packet[i] = read_reg(SI4430_FIFO);
			read_burst(SI4430_FIFO, pkg_len, packet);
			memcpy(packet_decoded, packet, 64);
			white(pkg_len, packet_decoded);
			memcpy(packet_unwhite, packet_decoded, 20);
			if (packet_decoded[0] + 3 < pkg_len) // avoid accessing invalid memory (3 = 1 byte length + 2 byte crc)
				pkg_len = packet_decoded[0] + 3;
			crc = crc16(0xffff, pkg_len, packet_decoded);
			decode(pkg_len - 3, packet_decoded + 1);
			if (crc != 0)
				__asm__("break");

			pkg_len = sizeof(testpkg);
			testpkg[0] = pkg_len - 3;
			testpkg[1] = packet_decoded[1]; // answer with same seq-id
			encode(pkg_len - 3, testpkg + 1);
			crc = crc16(0xffff, pkg_len - 2, testpkg);
			testpkg[pkg_len - 2] = crc >> 8;
			testpkg[pkg_len - 1] = crc & 0xff;
			crc = crc16(0xffff, pkg_len, testpkg);
			if (crc != 0)
				__asm__ ("break");
			white(pkg_len, testpkg);

			write_reg(SI4430_OMFC2, SI4430_FFCLRRX | SI4430_FFCLRTX); // clear fifos
			write_reg(SI4430_OMFC2, 0);
			write_reg(SI4430_PKLEN, pkg_len);
			write_burst(SI4430_FIFO, pkg_len, testpkg);
			write_reg(SI4430_OMFC1, SI4430_TXON); // enter transmit mode

			white(pkg_len, testpkg);
			crc = crc16(0xffff, pkg_len, testpkg);
			decode(pkg_len - 3, testpkg + 1);

			//__asm__("break");
		}
		else if (status1 & SI4430_PKGSENT) {
			write_reg(SI4430_OMFC2, SI4430_FFCLRRX | SI4430_FFCLRTX); // clear fifos
			write_reg(SI4430_OMFC2, 0);
			write_reg(SI4430_PKLEN, 14 + 3); // TODO
			write_reg(SI4430_RXAFTHR, 15); // TODO
			write_reg(SI4430_OMFC1, SI4430_RXON); // enter receive mode
		}
	}
}


void spi_enable()
{
	CLK_PCKENR1 |= CLK_PCKENR1_SPI;
	SPI1_CR1 |= SPI_CR1_SPE; // enable spi
}

void spi_disable()
{
	while (!(SPI1_SR & SPI_SR_TXE) || (SPI1_SR & SPI_SR_BSY))
		;
	SPI1_CR1 &= ~SPI_CR1_SPE;
	CLK_PCKENR1 &= ~CLK_PCKENR1_SPI;
}

static uint8_t send_byte(uint8_t data)
{
	//while (!(SPI1_SR & SPI_SR_TXE) || (SPI1_SR & SPI_SR_RXNE) || (SPI1_SR & SPI_SR_BSY))
	//	(void)SPI1_DR;
	SPI1_DR = data;
	while (!(SPI1_SR & SPI_SR_RXNE))
		;
	return SPI1_DR;
}
static void strobe(uint8_t cmd)
{
	select();
	wait_miso();
	status = send_byte(cmd);
	deselect();
	_delay_us(1000);
}

static uint8_t read_reg(uint8_t addr)
{
	uint8_t val;

	select();
	//wait_miso(); // only cc1101
	send_byte(addr | READ_BURST);
	val = send_byte(0x0);
	deselect();

	return val;
}

static void read_burst(uint8_t addr, uint8_t len, uint8_t * data)
{
	select();
	send_byte(addr | READ_BURST);
	for ( ; len; len--)
		*(data++) = send_byte(0x0);
	deselect();
}

static void write_reg(uint8_t addr, uint8_t data)
{
	select();
	send_byte(addr | WRITE_BURST);
	send_byte(data);
	deselect();
}

static void write_burst(uint8_t addr, uint8_t len, uint8_t * data)
{
	select();
	send_byte(addr | WRITE_BURST);
	for ( ; len; len--)
		send_byte(*(data++));
	deselect();
}
