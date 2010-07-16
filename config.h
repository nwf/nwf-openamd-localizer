#ifndef _CONFIG_H_
#define _CONFIG_H_

#define DEBUG					0

#define BADGE_MINIMUM_ID		3000	/* Inclusive */
#define BADGE_MAXIMUM_ID		8000	/* Inclusive */

#define HISTORY_WINDOW_SIZE		32	
	/** Optionally workaround a bug whereby sequence IDs are
	 * not held constant within a burst of identical packets
	 *
	 * Set to 0 to disable workaround, or 2 to ignore the bottom
	 * two bits of all sequence IDs, since badges send 4 packets
	 * at a time.
	 */
#define CONFIG_SEQUENCE_ID_SHIFT	2

#endif
