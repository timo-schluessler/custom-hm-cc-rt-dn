#ifndef AS_INCLUDED
#define AS_INCLUDED

#define AS_HEADER_SIZE 9 // header length without length byte

const uint8_t * hm_id = EEPROM_START; // 3 byte
const uint8_t * hm_serial = EEPROM_START + 3; // 10 byte
const uint8_t * hm_master_id = EEPROM_START + 13; // 3 byte

typedef union {
	struct {
		uint8_t length;
		uint8_t counter;
		uint8_t flags;
		uint8_t type; 
		uint8_t from[3];
		uint8_t to[3];
		uint8_t payload[MAX_PAYLOAD];
		uint8_t crc[2]; // this is only to reserve the space! the crc is not really contained in this field!
	};
	uint8_t data[];
} as_packet_t;

#define AS_FLAG_DEF 0x80
#define AS_FLAG_BIDI 0x20

#define CMP_ID(a, b) (((a)[0] == (b)[0] && (a)[1] == (b)[1] && (a)[2] == (b)[2]) ? 0 : 1)
#define LIST_ID(a) (a)[0], (a)[1], (a)[2]
#define LIST_SERIAL(a) (a)[0], (a)[1], (a)[2], (a)[3], (a)[4], (a)[5], (a)[6], (a)[7], (a)[8], (a)[9]

void as_send_device_info();

#endif // AS_INCLUDED
