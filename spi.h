#ifndef SPI_INCLUDED
#define SPI_INCLUDED

#define nSEL (1u<<3)
#define nIRQ (1u<<4)
#define MISO (1u<<7)
#define SCLK (1u<<5)
#define MOSI (1u<<6)
#define select() do { PB_ODR &= ~nSEL; } while(false)
#define deselect() do { PB_ODR |= nSEL; } while(false)
#define wait_miso() while (PB_IDR & MISO);

void spi_init();
void spi_enable();
void spi_disable();
uint8_t spi_send_byte(uint8_t data);

#endif
