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

