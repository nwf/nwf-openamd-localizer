#ifndef _READERLOC_H_
#define _READERLOC_H_

#include <glib/ghash.h>
#include "dispatch.h"

typedef struct {
	rx_id id;
	double rx, ry, rz;
} rx_loc;

GHashTable *load_reader_location_data(void);

#endif
