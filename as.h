#ifndef AS_INCLUDED
#define AS_INCLUDED

#define MAX_PAYLOAD 10

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

#endif // AS_INCLUDED
