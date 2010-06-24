#include <stdint.h>
#include <stdio.h>
#include <string.h>
#include <arpa/inet.h>
#include <netinet/in.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <assert.h>

typedef unsigned u_int;		// Hate everything ever...
typedef unsigned short u_short;
typedef unsigned char u_char;

#include <pcap/pcap.h>

#include "config.h"
#include "dispatch.h"
#include "pcap_rx.h"
#include "util.h"

/////////////////// Hackish PCAP file reader

#define ETHER_SIZE	14

static void
handler(u_char *user, const struct pcap_pkthdr *h, const u_char *link_bytes) {
	if(h->caplen < ETHER_SIZE + 20 + 8 + 4) { return; } /* Ethernet + IP + UDP + DATA */
	const u_char *bytes = &link_bytes[ETHER_SIZE];
	if((bytes[0] & 0xF0) != 0x40) { return; } /* not IPv4 */
	int ipihl = (bytes[0] & 0x0F) * 4; /* length of IP header == offset of UDP header */
	if(bytes[6] != 0x00 || bytes[7] != 0x00) { return; } /* fragmented */
	if(bytes[9] != 0x11) { return; } /* not UDP */
	rx_id rx = demarshal32((uint8_t*) &bytes[12]);
	int datoff = ETHER_SIZE + ipihl + 8;  /* skip UDP header */
	dispatch_packets((dispatch_data*) user,
						(uint8_t *) &link_bytes[datoff],
						h->caplen-datoff,
						rx,
						&h->ts
	);
	return;
}

void
pcap_dispatch_file(dispatch_data *dd, FILE* f) {
	char errbuf[PCAP_ERRBUF_SIZE];

	pcap_t *pf = pcap_fopen_offline(f, errbuf);
	pcap_loop(pf, -1, handler, (u_char *) dd);
}
