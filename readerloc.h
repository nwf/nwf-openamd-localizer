#ifndef _READERLOC_H_
#define _READERLOC_H_

#include <stdio.h>
#include <glib/ghash.h>
#include "dispatch.h"

typedef struct {
	rx_id id;
	double weight;
	double rx, ry, rz;
} rx_loc;

GHashTable * reader_location_new_table(void);
void reader_location_cleanup(GHashTable **);
void reader_location_load_data(FILE *, GHashTable *);

#endif
