/**
 * A 3-dimensional space partition tree and serialized format.
 *
 * The serialized format is a series of lines, each of which look like
 *  [LABEL] [TYPE] ...\n
 * LABEL is a unique space-free token of your choosing, and TYPE is one of
 *  'B' for a Binary decision node
 *  'L' for a Label node
 *
 * Label nodes are trivial:
 *  [LABEL] L [PRINTLABEL]\n
 * where PRINTLABEL is the string returned by this node.
 *
 * Binary decision nodes less so:
 *  [LABEL] B [VAR] [COMP] [VALUE] [TRUE] [FALSE]\n
 * where
 *  VAR is one of {X, Y, Z},
 *  COMP is one of {<, <=, >=, >},
 *  VALUE is a floating point value suitable for strtod,
 *  and TRUE and FALSE are LABELs previously assigned to nodes.
 *
 */

#ifndef _SPACEPARTTREE_H_
#define _SPACEPARTTREE_H_

#include <stdio.h>

typedef struct spt spt;

spt *spt_read_from_file(FILE *);
void spt_print(spt *, FILE *);

void spt_cleanup(spt **);

char *spt_label(spt *, double, double, double);

#endif
