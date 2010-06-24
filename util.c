#include <stdint.h>
#include <stdlib.h>
#include <arpa/inet.h>

#include "util.h"

uint16_t
crc16 (uint8_t *buffer, int size)
{
  uint16_t crc = 0xFFFF;

  if (buffer && size)
    while (size--)
      {
        crc = (crc >> 8) | (crc << 8);
        crc ^= *buffer++;
        crc ^= ((unsigned char) crc) >> 4;
        crc ^= crc << 12;
        crc ^= (crc & 0xFF) << 5;
      }

  return crc;
}

#define MX  ( (((z>>5)^(y<<2))+((y>>3)^(z<<4)))^((sum^y)+(tea_key->keybits[(p&3)^e]^z)) )
#define DELTA 0x9e3779b9UL

void
xxtea_decode (uint32_t * v, uint32_t length, const xxteakey *tea_key)
{
  uint32_t z, y = v[0], sum = 0, e, p, q;

  if (!v || !tea_key)
    return;

  q = 6 + 52 / length;
  sum = q * DELTA;
  while (sum)
    {
      e = (sum >> 2) & 3;
      for (p = length - 1; p > 0; p--)
        z = v[p - 1], y = v[p] -= MX;

      z = v[length - 1];
      y = v[0] -= MX;
      sum -= DELTA;
    }
}

void
shuffle_tx_byteorder (uint32_t * v, uint32_t len)
{
  while (len--)
    {
		/* XXX This appears to be a case of misusing the hton* functions
		 * and so I suspect this code wouldn't work on big-endian machines.
		 * Go figure.  In any case, it'll get fixed eventually, but for now
		 * this is here just because it was this way.
		 */
      *v = htonl (*v);
      v++;
    }
}

uint16_t read16(uint8_t **b) {
	uint8_t *d = *b; *b += 2;
	return demarshal16(d);
}

uint16_t demarshal16(uint8_t *d) {
	return (*d << 8) | (*(d+1));
}

uint32_t read32(uint8_t **b) {
	uint8_t *d = *b; *b += 4;
	return demarshal32(d);
}

uint32_t demarshal32(uint8_t *d) {
	return (*d << 24) | (*(d+1) << 16) | (*(d+2) << 8) | (*(d+3));
}
