#include <linux/netlink.h>
#include <linux/if_addr.h>
#include <linux/rtnetlink.h>
#include <sys/socket.h>
#include <net/if.h>
#include <arpa/inet.h>
#include <string.h>

#include "addr.h"

extern uint8_t * network_ip_addr_to_broadcast(uint8_t * in, uint32_t len, uint32_t subnetmasklen, uint32_t * out) {
    uint32_t subnetmask = 0;
    for(int32_t i = 0; i < subnetmasklen; i++) {
        subnetmask |= (1 << i);
    }

    *out = ((*((uint32_t *) in)) & subnetmask) | (~subnetmask);

    return (uint8_t *) out;
}
