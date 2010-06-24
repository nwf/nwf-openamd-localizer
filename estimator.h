#ifndef _ESTIMATOR_H_
#define _ESTIMATOR_H_

#include <stdint.h>
#include "config.h"
#include "readerloc.h"

typedef struct {
	int denom;
	double sumx;
	double sumy;
	double sumz;
} est_badge_hist_cell;

typedef struct {
	est_badge_hist_cell cells[HISTORY_WINDOW_SIZE];
	int head_posn;			/* Position into the ring buffer */
	uint32_t head_seqid;	/* last observed sequence ID */

	double sumx;			/* Total over the whole FIFO */
	double sumy;
	double sumz;
	int denom;
} est_badge_data ;

void update_badge_pos(est_badge_data *, uint32_t, rx_loc*, uint8_t);

#endif
