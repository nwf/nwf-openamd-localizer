/** Reader location database. */

#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>
#include <sys/types.h>

#include "dispatch.h"
#include "readerloc.h"

#define IPv4(a,b,c,d) ( ((uint32_t)a)<<24 | ((uint32_t)b)<<16 | ((uint32_t)c)<<8 | ((uint32_t)d)<<0 )

#if 0
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
#endif

GHashTable *reader_location_new_table(void) {
	return g_hash_table_new_full(g_int_hash, g_int_equal, NULL, free);
}

void reader_location_cleanup(GHashTable **t) {
	if(*t) g_hash_table_destroy(*t);
	*t = NULL;
}

void reader_location_load_data(FILE *f, GHashTable *ret) {

	int a,b,c,d;
	rx_loc *rl = calloc(1, sizeof(rx_loc));
	while(fscanf(f, "%d.%d.%d.%d %lf %lf %lf\n",
		&a,&b,&c,&d,&rl->rx,&rl->ry,&rl->rz
	) == 7) {
		rl->id = IPv4(a,b,c,d);
		rl->weight = 1.0;
		g_hash_table_insert(ret, (gpointer) &rl->id, (gpointer) rl);
		rl = calloc(1, sizeof(rx_loc));
	}
	free(rl);
}
