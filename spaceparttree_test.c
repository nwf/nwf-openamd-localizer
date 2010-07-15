#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <glib/ghash.h>

#include "spaceparttree_priv.h"
#include "spaceparttree.h"

int
main() {
	spt *spt = spt_read_from_file(stdin);
	if(!spt) { return -1; }

	printf("0,0,0 = %s\n",  spt_label(spt, 0, 0, 0) );
	printf("10,0,0 = %s\n", spt_label(spt, 10, 0, 0));
	
	spt_print(spt, stdout);
	spt_cleanup(&spt);
}
