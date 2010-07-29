#include <alloca.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>

#include "config.h"
#include "dispatch.h"
#include "openbeacon.h"
#include "util.h"

static const xxteakey cryptokeys[] =
    { { "TLH"      , { 0x9c43725e, 0xad8ec2ab, 0x6ebad8db, 0xf29c3638 } } 
    , { "25C3 beta", { 0x7013F569, 0x4417CA7E, 0x07AAA968, 0x822D7554 } }
    , { "Camp 2007", { 0x8e7d6649, 0x7e82fa5b, 0xddd4541e, 0xe23742cb } }  
    , { "UNK 1"    , { 0xab94ec75, 0x160869c5, 0xfbf908da, 0x60bedc73 } }
    , { "24C3 key" , { 0xb4595344, 0xd3e119b6, 0xa814d0ec, 0xeff5a24e } }
    , { "25C3 real", { 0xbf0c3a08, 0x1d4228fc, 0x4244b2b0, 0x0b4492e9 } }
    , { "23C3 ?"   , { 0xdeadbeef, 0xdeadbeef, 0xdeadbeef, 0xdeadbeef } } 
    , { "23C3 key" , { 0xe107341e, 0xab99c57e, 0x48e17803, 0x52fb4d16 } } 
    , { "UNK 2"    , { 0xee2522d1, 0xdbc221f1, 0xa21d0d0e, 0x865976a2 } }
	, { "Fs"       , { 0xFFFFFFFF, 0xFFFFFFFF, 0xFFFFFFFF, 0xFFFFFFFF } }
	};
#define NR_CRYPTKEYS (sizeof cryptokeys)/(sizeof cryptokeys[0])

static int
wellformed(uint8_t *s, uint8_t *e) {
	uint8_t pl = *s;			/* Read a length byte */
	if(pl < 1+1+2) {
		if(DEBUG) fprintf(stderr,
							"Packet claimed length is short\n"
						);
		return 0;
	}

	if(s + pl > e) {
		/* Packet too big? */
		if(DEBUG) fprintf(stderr,
							"Discarding junk at end of buffer\n"
						);
		return 0;
	}

	uint16_t c_claim = (*(s + pl - 2) << 8) + *(s + pl - 1);
	uint16_t c_calcd = crc16(s, pl-2);

	if(c_claim != c_calcd) {
		if(DEBUG) fprintf(stderr,
							"Invalid checksum (%x should be %x)!\n", c_claim, c_calcd
						);
		return 0;
	}

	return 1;
}

void
dispatch_set_callback(dispatch_data *dd,
						uint8_t type, dispatch_callback cb, void *data) {
	dd->dcbs[type].cb = cb;
	dd->dcbs[type].data = data;
}

void
dispatch_set_omni_callback(dispatch_data *dd, dispatch_callback cb, void *data) {
	dd->omni_cb = cb;
	dd->omni_data = data;
}

void
dispatch_set_backoff_callback(dispatch_data *dd, dispatch_callback cb, void *data) {
	dd->backoff_cb = cb;
	dd->backoff_data = data;
}

static void
dispatch(dispatch_data *dd, uint8_t *s, dispatch_rx_info *rxi, const struct timeval *tv)
{
	uint8_t type = *(s+1);

	if(dd->omni_cb != NULL)
		dd->omni_cb(dd->omni_data, s, rxi, tv);

	if(dd->dcbs[type].cb != NULL)
		dd->dcbs[type].cb(dd->dcbs[type].data, s, rxi, tv);
	else if (dd->backoff_cb != NULL)
		dd->backoff_cb(dd->backoff_data, s, rxi, tv);
}

#define XXTEA_TRY_LEN	16

void
dispatch_packets(dispatch_data *dd,
				uint8_t *_buf, ssize_t pktsize,
				rx_id src, const struct timeval *tv)
{

	uint8_t *s = _buf;
	uint8_t *e = &_buf[pktsize];

	dispatch_rx_info rxi;
	rxi.rxid = src;

	toploop:
	if(DEBUG) fprintf(stderr, "buf (%ld) = %2x %2x %2x %2x\n", e-s, s[0], s[1], s[2], s[3]);

	while(s + 4 < e) {
		uint8_t pl = *s;			/* Read a length byte */

		rxi.keyid = -1;

		if(wellformed(s,e)) {
			dispatch(dd, s, &rxi, tv);
			s += pl;	/* Move to next packet */
		} else {
			if(e-s >= XXTEA_TRY_LEN) {
				/* Try cryptography.  Man what a hack */
				for(int ki = 0; ki < NR_CRYPTKEYS; ki++) {
					rxi.keyid = ki;
					uint8_t *sbuf = alloca(XXTEA_TRY_LEN);
					if(!sbuf) {
						if(DEBUG) fprintf(stderr, "Cannot allocate on stack!");
						return;
					}
					memmove(sbuf, s, XXTEA_TRY_LEN);

					shuffle_tx_byteorder((uint32_t*) sbuf, XXTEA_TRY_LEN/4);
					xxtea_decode((uint32_t*)sbuf, XXTEA_TRY_LEN/4, &cryptokeys[ki]);
					shuffle_tx_byteorder((uint32_t*)sbuf, XXTEA_TRY_LEN/4);

					if(DEBUG) {
					fprintf(stderr, "dec = %2.2x %2.2x %2.2x %2.2x", sbuf[0], sbuf[1], sbuf[2], sbuf[3]);
					fprintf(stderr, " %2.2x %2.2x %2.2x %2.2x", sbuf[4], sbuf[5], sbuf[6], sbuf[7]);
					fprintf(stderr, " %2.2x %2.2x %2.2x %2.2x", sbuf[8], sbuf[9], sbuf[10], sbuf[11]);
					fprintf(stderr, " %2.2x %2.2x %2.2x %2.2x\n", sbuf[12], sbuf[13], sbuf[14], sbuf[15]);
					}

					if(*sbuf == XXTEA_TRY_LEN
					&& wellformed(sbuf, sbuf+XXTEA_TRY_LEN)) {
						dispatch(dd, sbuf, &rxi, tv);
						s += XXTEA_TRY_LEN;
						goto toploop;
					}
				}
			}
			/* Packet not well formed and no cryptography we know how to do
			 * was able to save us.  Give up.
			 */
			break;	
		}
	}	
}

const char *
dispatch_keyname_by_id(int id)
{
	return cryptokeys[id].keyname;
}
