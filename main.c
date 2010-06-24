#define _GNU_SOURCE

#include <stdint.h>
#include <strings.h>
#include <sys/types.h>
#include <unistd.h>

#include <glib/ghash.h>

#include "dispatch.h"
#include "estimator.h"
#include "normalized_rxtx.h"
#include "openbeacon.h"
#include "pcap_rx.h"
#include "readerloc.h"

void
usage(void) {
	printf("OpenBeacon aggregator framework.  Command line usage:\n");
	printf("Packet sources: -P pcap_file, -N normalized_file, -U udp_port\n");
	printf("Packet logging: -O normalized_file\n");
	printf("OpenBeacon Tracker estimation output: -H human, -S structured\n");
}

int
main(int argc, char **argv) {
	dispatch_data dd;
	bzero(&dd, sizeof(dd));

	openbeacon_tracker_data btd;
	bzero(&btd, sizeof(btd));

	int do_tracker = 0;

	FILE *normout = NULL;

	FILE *sourcef = NULL;
	// int port = -1;
	enum { SOURCE_NONE
	     , SOURCE_NETWORK
	     , SOURCE_NORMALIZED
	     , SOURCE_PCAP }
	  source = SOURCE_NONE;

	int opt;
	while((opt = getopt(argc, argv, "H:N:O:P:S:U:h")) != -1) {
		switch (opt) {
		case 'H':
			do_tracker = 1;
			btd.human_out_file = fopen(optarg, "w");
			break;
		case 'N':
			source = SOURCE_NORMALIZED;
			if(sourcef) fclose(sourcef);
			sourcef = fopen(optarg, "r");
			break;
		case 'O':
			if(normout) fclose(normout);
			normout = fopen(optarg, "w");
			break;
		case 'P':
			source = SOURCE_PCAP;
			if(sourcef) fclose(sourcef);
			sourcef = fopen(optarg, "r");
			break;
		case 'S':
			do_tracker = 1;
			btd.structured_out_file = fopen(optarg, "w");
			break;
		case 'h':	usage(); return -1;
		}
	}

	if(source == SOURCE_NONE) {
		printf("No source specified; nothing to do.\n");
		return -1;
	}

	if(do_tracker) {
		// XXX
		btd.rxid_location = load_reader_location_data();
		btd.oid_estdata = g_hash_table_new(g_int_hash, g_int_equal);
		dispatch_set_callback(&dd, OPENBEACON_PROTO_BEACONTRACKER,
								beacontracker_cb, &btd);
	}
	if(normout != NULL) {
		dispatch_set_omni_callback(&dd, normalized_write_cb, normout); 
	}

	switch(source) {
	case SOURCE_PCAP: pcap_dispatch_file(&dd, sourcef); break;
	case SOURCE_NORMALIZED: normalized_dispatch_file(&dd, sourcef); break;
	default: printf("PANIC: Unknown source type?!\n"); return -1;
	}

	return 0;
}
