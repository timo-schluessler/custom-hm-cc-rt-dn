#ifndef AS_INCLUDED
#define AS_INCLUDED

#define AS_HEADER_SIZE 9 // header length without length byte

const uint8_t * hm_id = EEPROM_START; // 3 byte
const uint8_t * hm_serial = EEPROM_START + 3; // 10 byte
const uint8_t * hm_master_id = EEPROM_START + 13; // 3 byte
#ifndef BOOTLOADER // the following line is not optimized but treated as a real pointer. but in bootloader we can't use CONST code segment (because of needed far addressing mode which is unsupported by sdcc)
uint8_t * const min_battery_voltage = EEPROM_START + 16; // 1 byte
#endif

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

#ifndef BOOTLOADER
extern bool as_ok;
extern uint8_t as_valve_value;
extern uint16_t as_sleep_value;
#endif

#define AS_FLAG_DEF 0x80
#define AS_FLAG_BIDI 0x20

#define CMP_ID(a, b) (((a)[0] == (b)[0] && (a)[1] == (b)[1] && (a)[2] == (b)[2]) ? 0 : 1)
#define ID_IS_NULL(a) (((a)[0] == 0x0 && (a)[1] == 0x0 && (a)[2] == 0x0) ? true : false)
#define LIST_ID(a) (a)[0], (a)[1], (a)[2]
#define LIST_SERIAL(a) (a)[0], (a)[1], (a)[2], (a)[3], (a)[4], (a)[5], (a)[6], (a)[7], (a)[8], (a)[9]

void as_poll();
void as_send_device_info();
void as_listen();

#endif // AS_INCLUDED
