#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>

#include "config.h"
#include "openbeacon.h"
#include "estimation.h"

void
update_badge_pos(est_badge_data *data,	/* This badge */
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
			est_badge_hist_cell *cell = &data->cells[data->head_posn];
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
	est_badge_hist_cell *cell = &data->cells[index];

#define BUPD(x) do { \
		double incr = rxl->r##x * prox; \
		cell->sum##x += incr;           \
		data->sum##x += incr;           \
	} while(0)

	BUPD(x);
	BUPD(y);
	BUPD(z);
#undef BUPD

	cell->denom += prox;
	data->denom += prox;
}

