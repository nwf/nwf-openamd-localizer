/** Reader location database.
 *
 * Taken from OpenBeacon's database
 * 
 *  @bug Really ought to read from file rather than have a
 *  baked-in table.
 */

#include <stdint.h>
#include <sys/types.h>

#include "dispatch.h"
#include "readerloc.h"

#define IPv4(a,b,c,d) ( ((uint32_t)a)<<24 | ((uint32_t)b)<<16 | ((uint32_t)c)<<8 | ((uint32_t)d)<<0 )

static const rx_loc readers[] = {
  {IPv4 (10,254, 0, 60), 015.5, 005.5, 7.0 },
  {IPv4 (10,254, 0, 63), 000.5, 000.5, 10.0 },
  {IPv4 (10,254, 0, 64), 000.5, 005.5, 9.0 },
};

GHashTable * load_reader_location_data(void) {
	GHashTable *ret = g_hash_table_new(g_int_hash, g_int_equal);
	for(int i = 0; i < sizeof(readers)/sizeof(readers[0]); i++) {
		g_hash_table_insert(ret, (gpointer) &readers[i].id,
								(gpointer) &readers[i]);
	}

	return ret;
}
