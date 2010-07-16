#include <assert.h>
#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include <glib/ghash.h>

#include "dispatch.h"
#include "readerloc.h"
#include "openbeacon.h"
#include "spaceparttree.h"
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
print_badge_human_data(FILE *f, openbeacon_tracker_data *btd, openbeacon_badge *b)
{
	if(b->data.denom == 0) {
		printf("BADGE %d: NO DATA\n", b->id);
	}
#define BAVG(x) (b->data.sum##x / b->data.denom)
	fprintf(f, "BADGE %d: ESTIMATE AS OF %ld IS (%f, %f, %f) (/%d) [%s] (F%2.2x@%ld)\n",
		b->id, b->last_print_time.tv_sec,
		BAVG(x), BAVG(y), BAVG(z), b->data.denom,
		spt_label(btd->areaspt, BAVG(x), BAVG(y), BAVG(z)),
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
print_badge_structured_data(FILE *f, openbeacon_tracker_data *btd, openbeacon_badge *b)
{
	if(b->data.denom == 0) {
		return;
	}
#define BAVG(x) (b->data.sum##x / b->data.denom)
	fprintf(f, "%X %lX %lA %lA %lA %X %2.2X@%lX %s\n",
		b->id, b->last_print_time.tv_sec,
		BAVG(x), BAVG(y), BAVG(z), b->data.denom,
		b->last_touch_value, b->last_touch_time.tv_sec,
		spt_label(btd->areaspt, BAVG(x), BAVG(y), BAVG(z)));
#undef BAVG
}



static void
update_badge_pos(openbeacon_badge_data *data,	/* This badge */
					uint32_t seq,		/* This sequence number */
					rx_loc *rxl,		/* The reader location */
					uint8_t prox		/* Proximity multiplier */
				)
{
	int index;
	int seqdelta = seq - data->head_seqid;

	if(-seqdelta >= HISTORY_WINDOW_SIZE) {
		if(DEBUG) fprintf(stderr,
					"Beacon (%x) in far past (vs %x) for badge data at %p\n",
					seq, data->head_seqid, data);
		return;
	}


	if((seqdelta > 0) && seqdelta < HISTORY_WINDOW_SIZE) {
		if(DEBUG) fprintf(stderr, "NEAR FUTURE\n");
		/* Roll the buffer forward, inserting zero cells, then
		 * turn this into near past
		 */
		while(seqdelta--) {
			data->head_posn+=1;
			if(data->head_posn == HISTORY_WINDOW_SIZE) {
				data->head_posn = 0;
			}
			openbeacon_badge_hist_cell *cell = &data->cells[data->head_posn];
			data->denom -= cell->denom;
			data->sumx  -= cell->sumx;
			data->sumy  -= cell->sumy;
			data->sumz	-= cell->sumz;
			memset(cell, 0, sizeof(*cell));
		}
		data->head_seqid = seq;
		index = data->head_posn;
	} else if (seqdelta >= HISTORY_WINDOW_SIZE) {
		if(DEBUG) fprintf(stderr, "FAR FUTURE\n");
		/* discard everything, turn this into near past */
		memset(data, 0, sizeof(*data));
		data->head_seqid = seq;
		index = data->head_posn;
	} else /* near past */ {
		if(DEBUG) fprintf(stderr, "NEAR PAST %d\n", seqdelta);
		/* Index backwards, wrapping the fifo */
		if (-seqdelta > data->head_posn) {
			index = HISTORY_WINDOW_SIZE + seqdelta - data->head_posn;
		} else {
			index = data->head_posn + seqdelta;
		}
	}

	assert(index >= 0 && index < HISTORY_WINDOW_SIZE);

	if(DEBUG) fprintf(stderr,
				"Head at %d, Head seq %d, index %d\n",
				data->head_posn, data->head_seqid, index);
	openbeacon_badge_hist_cell *cell = &data->cells[index];

#define BUPD(x) do { \
		double incr = rxl->r##x * rxl->weight * prox; \
		cell->sum##x += incr;                         \
		data->sum##x += incr;                         \
	} while(0)

	BUPD(x);
	BUPD(y);
	BUPD(z);
#undef BUPD

	cell->denom += prox * rxl->weight;
	data->denom += prox * rxl->weight;
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
	update_badge_pos(&b->data, pp.seq >> CONFIG_SEQUENCE_ID_SHIFT,
						(rx_loc*)rxloc, prox);

	if(tv != NULL && pp.flags != 0xFF) {
		b->last_touch_time = *tv;
		b->last_touch_value = pp.flags;
	}

	if(tv == NULL || (tv->tv_sec - last_time.tv_sec >= 1)) {
		if(tv != NULL) b->last_print_time = *tv;
		if(btd->human_out_file)
			print_badge_human_data(btd->human_out_file, btd, b);
		if(btd->structured_out_file)
			print_badge_structured_data(btd->structured_out_file, btd, b);
	}
}

void beacontracker_init_data(openbeacon_tracker_data *btd) {
	btd->oid_estdata = g_hash_table_new_full(g_int_hash, g_int_equal, NULL, free);
	btd->rxid_location = reader_location_new_table();	
}

void beacontracker_cleanup_data(openbeacon_tracker_data *btd) {
	g_hash_table_destroy(btd->oid_estdata);
	btd->oid_estdata = NULL;
	reader_location_cleanup(&btd->rxid_location);
	spt_cleanup(&btd->areaspt);
}
