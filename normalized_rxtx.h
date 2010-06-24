#ifndef _NORMALIZED_RXTX_H_
#define _NORMALIZED_RXTX_H_

#include <stdint.h>
#include <stdio.h>
#include <sys/types.h>

#include "dispatch.h"

void normalized_write_cb(void *, uint8_t *, rx_id, const struct timeval *);
void normalized_dispatch_file(dispatch_data *, FILE *);

#endif
