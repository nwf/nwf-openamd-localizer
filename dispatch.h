#ifndef _FRAME_H_
#define _FRAME_H_

#include <stdint.h>
#include <sys/types.h>
#include <sys/time.h>

typedef uint32_t rx_id;

typedef struct {
		/** @brief Quasi-opaque key identifier used to decrypt this key.
		 *  @note  -1 for plaintext packets.
		 */
	int keyid;
	rx_id rxid;
} dispatch_rx_info;

typedef void (*dispatch_callback)(void *
                                 , uint8_t *
                                 , dispatch_rx_info *
                                 , const struct timeval *
                                 );

typedef struct {
	// XXX This could perhaps be a map rather than an array, but
	// 256 * 8 = 2048 which isn't too bad yet.
	struct {
		dispatch_callback cb;
		void *data;
	} dcbs[UINT8_MAX];

	dispatch_callback omni_cb;	/**< Callback called on every packet */
	void *omni_data;			/**< Data for omni callback */

	dispatch_callback backoff_cb;	/**< Callback fired if no registered cb */
	void *backoff_data;				/**< Data for backoff callback */
} dispatch_data;


void dispatch_set_callback(dispatch_data *, uint8_t,
						dispatch_callback, void *);
void dispatch_set_omni_callback(dispatch_data *, dispatch_callback, void *);
void dispatch_set_backoff_callback(dispatch_data *,	dispatch_callback, void *);
void dispatch_packets(dispatch_data *, uint8_t *,
						ssize_t, rx_id, const struct timeval *);

const char *dispatch_keyname_by_id(int);

#endif
