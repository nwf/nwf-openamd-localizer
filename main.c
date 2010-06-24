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

int
main() {
	openbeacon_tracker_data btd;
	dispatch_data dd;

	bzero(&btd, sizeof(btd));
	bzero(&dd, sizeof(dd));

	// btd.structured_out_file = stdout;
	btd.human_out_file = stdout;

	btd.rxid_location = load_reader_location_data();
	btd.oid_estdata = g_hash_table_new(g_int_hash, g_int_equal);
	dispatch_set_callback(&dd, OPENBEACON_PROTO_BEACONTRACKER, beacontracker_cb, &btd);
	// dispatch_set_omni_callback(&dd, normalized_write_cb, fopen("./test.norm", "w")); 

	pcap_dispatch_file(&dd, fopen("./test.cap", "r"));
	// normalized_dispatch_file(&dd, fopen("./test.norm", "r"));
}
