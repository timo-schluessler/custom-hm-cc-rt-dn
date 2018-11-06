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

#ifndef BOOTLOADER
#define FLASH_START 0x8000

static uint8_t as_cnt = 0;

static bool as_config_start(uint8_t channel, uint8_t list);
static bool as_config_write(uint8_t channel, uint8_t length, uint8_t * data);
static bool as_config_end(uint8_t channel);

#define MAX_ACK_LENGTH 10

static as_packet_t ack_packet;
bool as_send(as_packet_t * packet)
{
	bool ok = false;

	//spi_enable();
	for (int try = 0; try < 3; try++) {
		uint16_t timeout_at;

		radio_send(packet);
		if (!radio_wait(get_tick() + 1000))
			__asm__("break\n"); // TODO

		if (!(packet->flags & AS_FLAG_BIDI)) {
			ok = true;
			break; // if not bidirectional don't wait for ack
		}

		timeout_at = get_tick() + 300;
		while (!tick_elapsed(timeout_at)) {

			radio_enter_receive(MAX_ACK_LENGTH);
			if (!radio_wait(timeout_at))
				continue;
		
			if (!radio_receive(&ack_packet, MAX_ACK_LENGTH)) {
				__asm__ ("break");
				continue; // if crc check fails, continue receiving
			}
			if (CMP_ID(ack_packet.to, packet->from) != 0) {
				__asm__ ("break");
				continue; // not for us
			}
			if (CMP_ID(ack_packet.from, packet->to) != 0)
				continue; // not from our target
			if (ack_packet.counter != packet->counter)
				continue; // answer to wrong/another message
			if (ack_packet.type != 0x02)
				continue; // not an ack packet

			ok = true;
			break;
		}
		if (ok)
			break;
	}

	//spi_disable();

	return ok;
}

void as_send_device_info()
{
	as_packet_t dev_info = { .length = 26, .counter = as_cnt++, .flags = AS_FLAG_DEF, .type = 0x00,
	                         .from = { LIST_ID(hm_id) }, .to = { LIST_ID(hm_master_id) },
	                         .payload = {
	                            0x01, // fw version
	                            0x8F, 0xFD, // model version
	                            LIST_SERIAL(hm_serial),
	                            0x30, 0x00, 0x00, 0x00 // unknown?
	                       }};
	if (!ID_IS_NULL(hm_master_id))
		dev_info.flags |= AS_FLAG_BIDI;

	as_send(&dev_info);
}

#define CONFIG_START_LENGTH 16
#define CONFIG_WRITE_LENGTH 60
#define CONFIG_END_LENGTH 11
#define MAX_LENGTH CONFIG_WRITE_LENGTH

void as_listen()
{
	uint16_t timeout_at = get_tick() + 5000;

	spi_enable();
	while (!tick_elapsed(timeout_at)) {
		bool ack = false;
		as_packet_t packet;
		bool enter_bootloader = false;

		radio_enter_receive(MAX_LENGTH);
		if (!radio_wait(timeout_at))
			return;
	
		if (!radio_receive(&packet, MAX_LENGTH)) {
			__asm__ ("break");
			continue; // if crc check fails, continue receiving
		}

		if (CMP_ID(packet.to, hm_id) != 0) {
			__asm__ ("break");
			continue; // not for us
		}
		if (!ID_IS_NULL(hm_master_id) && CMP_ID(packet.from, hm_master_id) != 0)
			continue; // not from our master

		if (packet.counter != as_cnt) {
			//__asm__ ("break");
			//continue;
			// TODO ??
		}
		as_cnt++;

		if (packet.type == 0x01 && packet.length >= AS_HEADER_SIZE + 2 && packet.payload[1] == 0x05)
			ack = as_config_start(packet.payload[0], packet.payload[6]); // packet.payload[2-5] is peer id if configuring peer config
		else if (packet.type == 0x01 && packet.length >= AS_HEADER_SIZE + 2 && packet.payload[1] == 0x08)
			ack = as_config_write(packet.payload[0], packet.length - AS_HEADER_SIZE - 2, packet.payload + 2);
		else if (packet.type == 0x01 && packet.length >= AS_HEADER_SIZE + 2 && packet.payload[1] == 0x06)
			ack = as_config_end(packet.payload[0]);
		else if (packet.type == 0x11 && packet.length >= AS_HEADER_SIZE + 1 && packet.payload[0] == 0xca) { // enter bootloader
			enter_bootloader = true;
			ack = true;
		}
		
		// conf readback
		// send current data -> recieve valve position and sleep duration


		if (packet.flags & AS_FLAG_BIDI) { // send answer
			as_packet_t dev_info = { .length = 10, .counter = packet.counter, .flags = AS_FLAG_DEF, .type = 0x02,
											 .from = { LIST_ID(hm_id) }, .to = { LIST_ID(packet.from) },
											 .payload = {
												 ack ? 0x00 : 0x80
										  }};
			as_send(&dev_info);
		}

		if (enter_bootloader) {
			__asm__ ("rim\n"); // disable interrupts
			PC_CR2 = 0; // disable wheel interrupts
			PF_CR2 = 0; // disable button interrupts
			RST_SR = 0xff; // reset all reset sources to signal bootloader that we jumped into it
			__asm
			jpf	[FLASH_START + 1] // jump far because bootloader is in upper memory region
			__endasm;
		}
	}
}

struct {
	uint8_t channel;
	uint8_t list;
	bool active;
} config_data = { .active = false };
bool as_config_start(uint8_t channel, uint8_t list)
{
	config_data.channel = channel;
	config_data.list = list;
	config_data.active = true;

	return true;
}

bool as_config_write(uint8_t channel, uint8_t length, uint8_t * data)
{
	if (!config_data.active || config_data.channel != channel)
		return false;

	// enable eeprom programming
	FLASH_DUKR = 0xAE;
	FLASH_DUKR = 0x56;
	while (!(FLASH_IAPSR & FLASH_IAPSR_DUL))
		;

	for (uint8_t i = 0; i + 1 < length; i += 2) {
		if (config_data.channel == 0 && config_data.list == 0) {
			if (data[i] == 0x0a)
				((uint8_t*)hm_master_id)[0] = data[i + 1];
			else if (data[i] == 0x0b)
				((uint8_t*)hm_master_id)[1] = data[i + 1];
			else if (data[i] == 0x0c)
				((uint8_t*)hm_master_id)[2] = data[i + 1];
			else
				continue; // don't wait for write completion
		} else
			continue; // don't wait for write completion

		// wait for eeprom write completion
		while (!(FLASH_IAPSR & FLASH_IAPSR_EOP))
			;
	}

	FLASH_IAPSR &= ~FLASH_IAPSR_DUL;

	return true;
}

bool as_config_end(uint8_t channel)
{
	bool ack = config_data.active && config_data.channel == channel;
	config_data.active = false;

	return ack;
}

#endif // not BOOTLOADER
