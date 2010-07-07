#include <stdint.h>
#include <stdio.h>
#include <string.h>
#include <arpa/inet.h>
#include <netinet/in.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <assert.h>

#include "dispatch.h"
#include "network_rx.h"

/////////////////// BSD Network Stack code

#if 0
static int
sockaddr_in_node_comp(struct sockaddr_in *a, struct sockaddr_in *b) {
	if(a->sin_addr.s_addr < b->sin_addr.s_addr) return -1;
	if(a->sin_addr.s_addr > b->sin_addr.s_addr) return 1;
	return 0;
}
#endif

static int
network_init(int udp_port) {
	int netfd = socket (AF_INET, SOCK_DGRAM, IPPROTO_UDP);
	if(netfd == -1) {
		perror("socket: ");
		return -1;
	}

	struct sockaddr_in bsa;

	memset ((char *) &bsa, 0, sizeof (bsa));
    bsa.sin_family = AF_INET;
    bsa.sin_port = htons (udp_port);
    bsa.sin_addr.s_addr = htonl (INADDR_ANY);

    if (bind (netfd, (struct sockaddr *) &bsa, sizeof (bsa)) == -1) {
		perror("bind: ");
		return -1;
	}

	return netfd;
}

#define BUFLEN ((1<<16)-1)

void
network_loop(dispatch_data *dd, int udp_port) {
	uint8_t buf[BUFLEN];
	struct sockaddr_in src_addr;
	socklen_t slen = sizeof(src_addr);

	struct timeval tv;

	int netfd = network_init(udp_port);

	while(1) {
		ssize_t pktsize = recvfrom(netfd, buf, BUFLEN, 0,
									(struct sockaddr *)&src_addr,
									&slen);

		if(pktsize < 0) {
			perror("network_loop: recvfrom:");
			return;
		}

		gettimeofday(&tv, NULL);
		dispatch_packets(dd, buf, pktsize, ntohl(src_addr.sin_addr.s_addr), &tv);
	}
}


