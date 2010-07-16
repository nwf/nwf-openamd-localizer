#define _XOPEN_SOURCE 500

#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <glib/ghash.h>

#include "spaceparttree_priv.h"
#include "spaceparttree.h"

#define MAX_LINE_SIZE	256

static const char *spt_read_delim = " \t\r\n";

void spt_free_node(gpointer);

#define SPT_NODE_TYPE_BSP_CHAR 'B'
#define SPT_NODE_TYPE_LABEL_CHAR 'L'

spt *
spt_read_from_file(FILE *f)
{
	spt *ret = calloc(1,sizeof(spt));
	ret->nodebyname = g_hash_table_new_full(g_str_hash,
								g_str_equal,
								NULL,
								spt_free_node);

	int lineno = 0;
	char line[MAX_LINE_SIZE];
	char *toksave;
	spt_node_header *snh = NULL;

	if(!ret || !ret->nodebyname) { goto failoom; }

	while(fgets(line, MAX_LINE_SIZE, f) != NULL) {
		lineno++;
		snh = NULL;

		char *_nodelabel = strtok_r(line, spt_read_delim, &toksave);
		char *_nodetype = strtok_r(NULL, spt_read_delim, &toksave);

		/* Empty lines and comments */
		if(!_nodelabel || *_nodelabel == '#') { continue; }

		if(!_nodetype) {
			fprintf(stderr
				   ,"SPT reader aborting on line %d with malformed line.\n"
				   ,lineno);
			goto fail;
		}

		if(strlen(_nodetype) != 1) {
			fprintf(stderr
				   ,"SPT reader aborting on line %d with malformed type: '%s'\n"
				   ,lineno, _nodetype);
			goto fail;
		}


		switch(*_nodetype) {
		case SPT_NODE_TYPE_LABEL_CHAR: {
			spt_node_label *snl = calloc(1, sizeof(spt_node_label));
			snh = &snl->header;
			if(!snl) { goto failoom; }

			snl->printlabel = strdup(strtok_r(NULL, spt_read_delim, &toksave));
			if(!snl->printlabel) { goto failoom; }
			snl->header.nodetype = SPT_NODE_TYPE_LABEL;
			break; }
		case SPT_NODE_TYPE_BSP_CHAR: {
			spt_node_bsp *snb = calloc(1, sizeof(spt_node_bsp));
			snh = &snb->header;
			if(!snb) { goto failoom; }

			char *_var = strtok_r(NULL, spt_read_delim, &toksave);
			char *_comp = strtok_r(NULL, spt_read_delim, &toksave);
			char *_value = strtok_r(NULL, spt_read_delim, &toksave);
			char *_tname = strtok_r(NULL, spt_read_delim, &toksave);
			char *_fname = strtok_r(NULL, spt_read_delim, &toksave);

			if(!_fname) {
				fprintf(stderr
					   ,"SPT reader aborting on line %d"
						" with malformed line (too few parameters)\n"
					   ,lineno);
				goto fail;
			}

			if(strlen(_var) != 1) {
				fprintf(stderr
					   ,"SPT reader aborting on line %d"
						" with malformed variable: '%s'\n"
					   ,lineno, _var);
				goto fail;
			}
			switch(*_var) {
			case 'X': snb->var = SPT_VARIABLE_X; break;
			case 'Y': snb->var = SPT_VARIABLE_Y; break;
			case 'Z': snb->var = SPT_VARIABLE_Z; break;
			default :
				fprintf(stderr
					   ,"SPT reader aborting on line %d"
						" with unknown comparison variable: '%s'\n"
					   ,lineno, _comp);
				goto fail;
			}

			if(!strcmp(_comp, "<")) {
				snb->comp = SPT_COMP_LT;
			} else if (!strcmp(_comp, "<=")) {
				snb->comp = SPT_COMP_LE;
			} else if (!strcmp(_comp, ">=")) {
				snb->comp = SPT_COMP_GE;
			} else if (!strcmp(_comp, ">")) {
				snb->comp = SPT_COMP_GT;
			} else {
				fprintf(stderr
					   ,"SPT reader aborting on line %d"
						" with malformed comparison operator: '%s'\n"
					   ,lineno, _comp);
				goto fail;
			}

			char *endptr;
			snb->value = strtod(_value, &endptr);
			if(endptr == _value) {
				fprintf(stderr
					   ,"SPT reader aborting on line %d"
						" with malformed value operator: '%s'\n"
					   ,lineno, _value);
				goto fail;
			}

			snb->t = g_hash_table_lookup(ret->nodebyname, _tname);
			if(!snb->t) {
				fprintf(stderr
					   ,"SPT reader aborting on line %d"
						" with unknown node reference: '%s'\n"
					   ,lineno, _tname);
				goto fail;
			}

			snb->f = g_hash_table_lookup(ret->nodebyname, _fname);
			if(!snb->f) {
				fprintf(stderr
					   ,"SPT reader aborting on line %d"
						" with unknown node reference: '%s'\n"
					   ,lineno, _fname);
				goto fail;
			}

			snb->header.nodetype = SPT_NODE_TYPE_BSP;
			break; }
		default :
			fprintf(stderr
				   ,"SPT reader aborting on line %d with unknown type: '%s'\n"
				   ,lineno,_nodetype);
			goto fail;
		}

		snh->nodelabel = strdup(_nodelabel);
		if(!snh->nodelabel) { goto failoom; }
		if(g_hash_table_lookup(ret->nodebyname, snh->nodelabel)) {
			fprintf(stderr
				   ,"SPT reader aborting on line %d with duplicate node defn\n"
				   ,lineno);
			goto fail;
		}
		g_hash_table_insert(ret->nodebyname, snh->nodelabel, snh);
	}

	ret->root = snh;

	return ret;

failoom:
	fprintf(stderr, "SPT reader OOM on line %d!\n", lineno);
	goto fail;

fail:
	if(snh) free(snh);
	g_hash_table_destroy(ret->nodebyname);
	free(ret);
	return NULL;
}

static char
spt_variable_name(spt_variable v) {
	switch(v) {
	case SPT_VARIABLE_X: return 'X';
	case SPT_VARIABLE_Y: return 'Y';
	case SPT_VARIABLE_Z: return 'Z';
	default: assert(0);
	}
}

static char *
spt_comparison_name(spt_comparison c) {
	switch(c) {
	case SPT_COMP_LT: return "<";
	case SPT_COMP_LE: return "<=";
	case SPT_COMP_GE: return ">=";
	case SPT_COMP_GT: return ">";
	default: assert(0);
	}
}

static void
spt_print_internal(FILE *f, int pc, spt_node_header *snh) {

	if(snh->printcount == pc) { return; }
	snh->printcount = pc;

	switch(snh->nodetype) {
	case SPT_NODE_TYPE_BSP: {
		spt_node_bsp *snb = (spt_node_bsp*)snh;
		
		spt_print_internal(f, pc, snb->t);
		spt_print_internal(f, pc, snb->f);
		fprintf(f, "%s %c %c %s %g %s %s\n",
			snh->nodelabel,
			SPT_NODE_TYPE_BSP_CHAR,
			spt_variable_name(snb->var),
			spt_comparison_name(snb->comp),
			snb->value,
			snb->t->nodelabel,
			snb->f->nodelabel);
		break; }	
	case SPT_NODE_TYPE_LABEL:
		fprintf(f, "%s %c %s\n",
			snh->nodelabel,
			SPT_NODE_TYPE_LABEL_CHAR,
			((spt_node_label*)snh)->printlabel);
		break;
	default: assert(0);
	}
}

void
spt_print(spt *s, FILE *f)
{
	int pc = ++s->printcount;
	spt_print_internal(f, pc, s->root);
	fflush(f);
}

static spt_node_label *
spt_lookup_leaf(spt_variables *vs, spt_node_header *snh)
{
	switch(snh->nodetype) {
	case SPT_NODE_TYPE_LABEL:
		return ((spt_node_label*)snh);
	case SPT_NODE_TYPE_BSP: {
		spt_node_bsp *snb = (spt_node_bsp*)snh;

		double v1;
		switch(snb->var) {
		case SPT_VARIABLE_X: v1 = vs->x; break;
		case SPT_VARIABLE_Y: v1 = vs->y; break;
		case SPT_VARIABLE_Z: v1 = vs->z; break;
		default: assert(0);
		}

		spt_node_header *targ = snb->f;
		switch(snb->comp) {
		case SPT_COMP_LT: if(v1 < snb->value)  { targ = snb->t; }; break;
		case SPT_COMP_LE: if(v1 <= snb->value) { targ = snb->t; }; break;
		case SPT_COMP_GE: if(v1 >= snb->value) { targ = snb->t; }; break;
		case SPT_COMP_GT: if(v1 > snb->value)  { targ = snb->t; }; break;
		}

		return spt_lookup_leaf(vs, targ); }
	default :  assert(0);
	}
}

char *
spt_label(spt *s, double x, double y, double z)
{
	if(s == NULL) { return NULL; }

	spt_variables vs = {.x = x, .y = y, .z = z};
	spt_node_label *snl = spt_lookup_leaf(&vs, s->root);
	return snl->printlabel;
}

void
spt_free_node(gpointer _snh)
{
	spt_node_header *snh = (spt_node_header *)_snh;
	free(snh->nodelabel);
	switch(snh->nodetype) {
	case SPT_NODE_TYPE_LABEL:
		free(((spt_node_label*)snh)->printlabel);
		break;
	case SPT_NODE_TYPE_BSP:
		break;
	default :  assert(0);
	}
	free(snh);
}

void
spt_cleanup(spt **s)
{
	if(*s == NULL) { return; }

	g_hash_table_destroy((*s)->nodebyname);
	free(*s);
	*s = NULL;
}
