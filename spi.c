#include "spi.h"

void spi_init()
{
	PB_ODR &= ~SCLK;
	PB_CR1 = nSEL | SCLK | MOSI; // push-pull
	PB_DDR = nSEL | SCLK | MOSI; // output
	deselect();

	CLK_PCKENR1 |= CLK_PCKENR1_SPI;
	SPI1_CR2 = SPI_CR2_SSM | SPI_CR2_SSI; // software slave management: master
	SPI1_CR1 = 0b0*SPI_CR1_BR | SPI_CR1_MSTR; // f_sysclk / 2, master
}

uint8_t spi_send_byte(uint8_t data)
{
	//while (!(SPI1_SR & SPI_SR_TXE) || (SPI1_SR & SPI_SR_RXNE) || (SPI1_SR & SPI_SR_BSY))
	//	(void)SPI1_DR;
	SPI1_DR = data;
	while (!(SPI1_SR & SPI_SR_RXNE))
		;
	return SPI1_DR;
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


