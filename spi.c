#include "spi.h"

#include "delay.h"
#include "cc1101.h"

#include <stdint.h>
#include <stdbool.h>

#include "stm8l.h"

#define nSEL (1u<<3)
#define MISO (1u<<7)
#define select_cc() do { PB_ODR &= ~nSEL; } while(false)
#define deselect_cc() do { PB_ODR |= nSEL; } while(false)
#define wait_miso() while (PB_IDR & MISO);

static void strobe(uint8_t cmd);
static uint8_t read_reg(uint8_t addr);

static uint8_t status;

extern volatile uint8_t partnum;
extern volatile uint8_t version;

void spi_init()
{
	PB_CR1 = nSEL; // push-pull
	PB_DDR = nSEL; // output
	deselect_cc();

	SPI1_CR1 = 0*SPI_CR1_BR | SPI_CR1_MSTR; // f_sysclk / 2, master
	SPI1_CR2 = SPI_CR2_SSM | SPI_CR2_SSI; // software slave management: master

	spi_enable();

	// reset CC1101
	_delay_us(5);
	select_cc();
	_delay_us(10);
	deselect_cc();
	_delay_us(41);

	partnum = read_reg(CC1101_PARTNUM);
	version = read_reg(CC1101_VERSION);

	spi_disable();
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
	SPI1_DR = data;
	while (!(SPI1_SR & SPI_SR_RXNE))
		;
	return SPI1_DR;
}
static void strobe(uint8_t cmd)
{
	select_cc();
	wait_miso();
	status = send_byte(cmd);
	deselect_cc();
}

static uint8_t read_reg(uint8_t addr)
{
	uint8_t val;

	select_cc();
	wait_miso();
	status = send_byte(addr | READ_BURST);
	val = send_byte(0x0);
	deselect_cc();

	return val;
}
