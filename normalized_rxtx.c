/** A normalized, plaintext log format for easy testing and so on.
 *
 * This defines a simple header around OpenBeacon packets
 * which is sufficient to run a dispatch loop.  All fields
 * are space-separated, and records are newline separated.
 *  RXID           (%08X)
 *	TIME SEC	   (%016lX)
 *	TIME MICROSEC  (%05lX)
 *  OPENBEACON PKT (as a series of %2.2X values)
 *
 * All packets generated will be in plaintext and will be
 * well-formed (as defined by the checks in dispatch.c).
 */

#include <ctype.h>
#include <stdio.h>

#include "dispatch.h"
#include "util.h"

#define HEADER_FORMAT_WRITE_STRING "%08X %016lX %05lX "
#define HEADER_FORMAT_READ_STRING "%X %lX %lX "
#define BYTE_FORMAT_STRING "%2.2X"

void
normalized_write_cb(void *_f, uint8_t *bytes,
					dispatch_rx_info *rxi, const struct timeval *tv) {
	FILE *f = _f;

	rx_id rxid = rxi->rxid;

	fprintf(f, HEADER_FORMAT_WRITE_STRING, rxid, tv->tv_sec, tv->tv_usec);
	int len = *bytes;
	for(int i = 0; i < len; i++) {
		fprintf(f, BYTE_FORMAT_STRING, bytes[i]);
	}
	fprintf(f, "\n");
}

void
normalized_dispatch_file(dispatch_data *dd, FILE *f) {
	struct timeval tv;
	rx_id rxid;
	uint8_t pktbytes[UINT8_MAX];

	while(1) {
		if(fscanf(f, HEADER_FORMAT_READ_STRING, &rxid, &tv.tv_sec, &tv.tv_usec) != 3)
			return;

		int idx = 0;
		int c;
		do {
			c = fgetc(f);
			int v;
			if(('0' <= c) && (c <= '9')) { v = c - '0'; }
			else { v = c - 'A' + 0xA; }

			if(!(idx & 1)) { v *= 0x10; pktbytes[idx/2] = 0; }
			pktbytes[idx/2] |= v;

			idx++;
		} while(c != -1 && c != '\n');

		dispatch_packets(dd, pktbytes, pktbytes[0], rxid, &tv);
	}
	fclose(f);
}
