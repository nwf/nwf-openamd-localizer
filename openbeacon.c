#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>
#include <sys/types.h>
#include <glib/ghash.h>

#include "dispatch.h"
#include "estimation.h"
#include "openbeacon.h"
#include "util.h"

	/** Unmarshal a beacontracker packet into its logical form.
	 *  @param b   Byte stream
	 *  @param pkt Structure to receive demarshalled form.
	 */
static void
read_beacontracker(uint8_t *b, openbeacon_tracker_packet *pkt) {
	b += 2;		/* skip size, protocol */
	pkt->flags = *b++;
	pkt->strength = *b++;
	pkt->seq = read32(&b);
	pkt->oid = read32(&b);
	pkt->reserved = read16(&b);
}

	/** Print badge information in a vaguely-human-readable form */
static void
print_badge_human_data(FILE *f, openbeacon_badge *b)
{
	if(b->data.denom == 0) {
		printf("BADGE %d: NO DATA\n", b->id);
	}
#define BAVG(x) (b->data.sum##x / b->data.denom)
	fprintf(f, "BADGE %d: ESTIMATE AS OF %ld IS (%f, %f, %f) (/%d) (F%2.2x@%ld)\n",
		b->id, b->last_print_time.tv_sec,
		BAVG(x), BAVG(y), BAVG(z), b->data.denom,
		b->last_touch_value, b->last_touch_time.tv_sec);
#undef BAVG

	if(DEBUG) for(int i = 0; i < HISTORY_WINDOW_SIZE; i++) {
		fprintf(f, "\t%d:(%f,%f,%f)/%d\n", i,
			b->data.cells[i].sumx,
			b->data.cells[i].sumy,
			b->data.cells[i].sumz,
			b->data.cells[i].denom );
	}
}

	/** Print badge information in a structured way */
static void
print_badge_structured_data(FILE *f, openbeacon_badge *b)
{
	if(b->data.denom == 0) {
		return;
	}
#define BAVG(x) (b->data.sum##x / b->data.denom)
	fprintf(f, "%X %lX %lA %lA %lA %X %2.2X@%lX\n",
		b->id, b->last_print_time.tv_sec,
		BAVG(x), BAVG(y), BAVG(z), b->data.denom,
		b->last_touch_value, b->last_touch_time.tv_sec);
#undef BAVG
}


	/** The dispatch.c callback for OPENBEACON_PROTO_BEACONTRACKER.
	 *  @param d      User data, as given to dispatch_set_callback.
	 *  @param bytes  OpenBeacon packet byte stream.
	 *  @param rxer   Receiver identity.
	 *  @param tv     Timestamp of packet reception.
	 */
void
beacontracker_cb(void *d
                , uint8_t *bytes
                , dispatch_rx_info *rxi
                , const struct timeval *tv
                )
{
	openbeacon_tracker_data *btd = d;
	rx_id rxer = rxi->rxid;

	openbeacon_tracker_packet pp;
	read_beacontracker(bytes, &pp);

		/* Because nobody's been good about keeping their OID spaces
		 * separate, despite the whopping 32 bit space available to
		 * them, we at least ensure that our plaintext ones end up
		 * not getting stomped on by anybody else.
		 */
	int oid = pp.oid + ((rxi->keyid+1)<<16);

#if 0
	// Removed because we feel like tracking a lot more things.
	if ((oid > BADGE_MAXIMUM_ID) || (oid < BADGE_MINIMUM_ID)) {
		printf("Suspicious OID (outside designated range): %d\n", oid);
		return;
	}
#endif

	gpointer _b = g_hash_table_lookup(btd->oid_estdata, &oid);
	openbeacon_badge *b = _b;
	if(!_b) {
		b = calloc(1, sizeof(openbeacon_badge));
		b->id = oid;
		g_hash_table_insert(btd->oid_estdata, &b->id, (gpointer) b);
	}

	gpointer rxloc = g_hash_table_lookup(btd->rxid_location, &rxer);
	if(!rxloc) {
		printf("Discarding (unknown RX %8.8x)\n", rxer);
		return;
	}

	if(0) printf("RXer %8.8x saw %d (seq=%x, pl=%x))\n",
				rxer, oid, pp.seq, pp.strength);

	struct timeval last_time = b->last_print_time;

	uint8_t prox = ((~pp.strength) & 0xF) + 1;
	update_badge_pos(&b->data, pp.seq, (rx_loc*)rxloc, prox);

	if(tv != NULL && pp.flags != 0xFF) {
		b->last_touch_time = *tv;
		b->last_touch_value = pp.flags;
	}

	if(tv == NULL || (tv->tv_sec - last_time.tv_sec > 1)) {
		if(tv != NULL) b->last_print_time = *tv;
		if(btd->human_out_file)
			print_badge_human_data(btd->human_out_file, b);
		if(btd->structured_out_file)
			print_badge_structured_data(btd->structured_out_file, b);
	}
}
