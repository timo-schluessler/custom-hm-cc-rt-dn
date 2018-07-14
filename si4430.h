#ifndef SI4430_INCLUDED
#define SI4430_INCLUDED

#define READ_BURST 0x0
#define WRITE_BURST 0x80

#define SI4430_DEVICE_TYPE 0x0
#define SI4430_DEVICE_VERSION 0x1

#define SI4430_STATUS1 0x3
#define SI4430_FFERR (1u<<7)
#define SI4430_RXFFAFULL (1u<<4)
#define SI4430_PKGSENT (1u<<2)
#define SI4430_PKGVALID (1u<<1)

#define SI4430_STATUS2 0x4
#define SI4430_SWDET (1u<<7) // sync word detect
#define SI4430_PREAVAL (1u<<6) // pramble valid
#define SI4430_RSSI (1u<<4) // rssi above threshold
#define SI4430_CHIPRDY (1u<<1)

#define SI4430_INT1 0x5
#define SI4430_INT2 0x6

#define SI4430_OMFC1 0x7
#define SI4430_SWRES (1u<<7)
#define SI4430_TXON (1u<<3)
#define SI4430_RXON (1u<<2)
#define SI4430_XTON (1u<<0)

#define SI4430_OMFC2 0x8
#define SI4430_FFCLRRX (1u<<1)
#define SI4430_FFCLRTX (1u<<0)

#define SI4430_RSSITH 0x27
#define SI4430_PKLEN 0x3e

#define SI4430_RXAFTHR 0x7e
#define SI4430_FIFO 0x7f



#endif
