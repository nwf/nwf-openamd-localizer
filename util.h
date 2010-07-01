#ifndef _UTIL_H_
#define _UTIL_H_

#include <stdint.h>

typedef struct {
	char *keyname;
	uint32_t keybits[4];
} xxteakey;

uint16_t crc16(uint8_t *, int);
void xxtea_decode (uint32_t *, uint32_t, const xxteakey *);
void shuffle_tx_byteorder (uint32_t *, uint32_t);

uint16_t read16(uint8_t **);
uint16_t demarshal16(uint8_t *);
uint32_t read32(uint8_t **);
uint32_t demarshal32(uint8_t *);

#endif
