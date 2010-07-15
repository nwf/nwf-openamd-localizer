#ifndef _SPACEPARTTREE_PRIV_H_
#define _SPACEPARTTREE_PRIV_H_

#include <glib/ghash.h>

typedef enum {
	SPT_NODE_TYPE_BSP,
	SPT_NODE_TYPE_LABEL,
} spt_node_type;

typedef enum {
	SPT_VARIABLE_X,
	SPT_VARIABLE_Y,
	SPT_VARIABLE_Z,
} spt_variable;

typedef enum {
	SPT_COMP_LT,
	SPT_COMP_LE,
	SPT_COMP_GE,
	SPT_COMP_GT,
} spt_comparison;

typedef struct {
	char *nodelabel;
	spt_node_type nodetype;
	unsigned int printcount;
} spt_node_header;

	/* e.g. foo L unknown */
typedef struct {
	spt_node_header header;
	char *printlabel;
} spt_node_label;

	/* e.g. foo B x < 10.0 bar baz */
typedef struct {
	spt_node_header header;

	spt_variable var;
	spt_comparison comp;
	double value;

	spt_node_header *t;
	spt_node_header *f;
} spt_node_bsp;

typedef struct {
	double x;
	double y;
	double z;
} spt_variables ;

	/* The typedef for this is in the public header */
struct spt {
	GHashTable *nodebyname;
	spt_node_header *root;
	unsigned int printcount;
};

#endif
