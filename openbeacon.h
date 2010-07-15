#ifndef __OPENBEACON_H__
#define __OPENBEACON_H__

#include <stdint.h>
#include <stdio.h>
#include <sys/types.h>
#include <glib/ghash.h>
#include "config.h"
#include "dispatch.h"

#define OPENBEACON_PROTO_BEACONTRACKER	23

	/** This is the logical form of an OPENBEACON_PROTO_BEACONTRACKER packet.
	 *  @note It is NOT the on-the-wire format.
	 *  @see read_beacontracker.
	 */
typedef struct{
  uint8_t flags, strength;
  uint32_t seq;
  uint32_t oid;
  uint16_t reserved;
} openbeacon_tracker_packet;

typedef struct {
	int denom;
	double sumx;
	double sumy;
	double sumz;
} openbeacon_badge_hist_cell;

typedef struct {
	openbeacon_badge_hist_cell cells[HISTORY_WINDOW_SIZE];
	int head_posn;			/* Position into the ring buffer */
	uint32_t head_seqid;	/* last observed sequence ID */

	double sumx;			/* Total over the whole FIFO */
	double sumy;
	double sumz;
	int denom;
} openbeacon_badge_data ;

typedef struct {
	int id;
	uint8_t last_touch_value;
	struct timeval last_touch_time;
	struct timeval last_print_time;
	openbeacon_badge_data data;
} openbeacon_badge;

typedef struct {
  /* oid -> badge map */
  GHashTable *oid_estdata;
  /* rxer -> location map */
  GHashTable *rxid_location;

  FILE *human_out_file;
  FILE *structured_out_file;
} openbeacon_tracker_data;

void beacontracker_init_data(openbeacon_tracker_data *);
void beacontracker_cleanup_data(openbeacon_tracker_data *);
void beacontracker_cb(void*, uint8_t *, dispatch_rx_info *, const struct timeval *);

#endif/*__OPENBEACON_H__*/
