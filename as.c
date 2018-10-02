void decode(uint8_t len, uint8_t *buf)
{
	uint8_t i, t;
	uint8_t prev = buf[0];
	buf[0] = (~buf[0]) ^ 0x89;

	for (i = 1; i < len - 1; i++) {
		t = buf[i];
		buf[i] = (prev + 0xdc) ^ buf[i];
		prev = t;
	}

	buf[i] ^= buf[1];
}
void encode(uint8_t len, uint8_t *buf)
{
	uint8_t i;
	uint8_t buf2 = buf[1];
	uint8_t prev;

	buf[0] = (~buf[0]) ^ 0x89;
	prev = buf[0];

	for (i = 1; i < len - 1; i++) {
		prev = (prev + 0xdc) ^ buf[i];
		buf[i] = prev;
	}

	buf[i] ^= buf2;
}

// TODO ifndef BOOTLOADER??

static uint8_t as_cnt = 0;

void as_send_device_info()
{
	as_packet_t dev_info = { .length = 26, .counter = as_cnt, .flags = AS_FLAG_DEF, .type = 0x00,
	                         .from = { LIST_ID(hm_id) }, .to = { LIST_ID(hm_master_id) },
	                         .payload = {
	                            0x00, // fw version?
	                            0x8F, 0xFD, // model version
	                            LIST_SERIAL(hm_serial),
	                            0x30, 0x00, 0x00, 0x00 // unknown?
	                       }};

	//spi_enable();
	radio_send(&dev_info);
	if (!radio_wait(get_tick() + 1000))
		__asm__("break\n"); // TODO
	//spi_disable();
}
