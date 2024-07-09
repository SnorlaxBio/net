#include <linux/netlink.h>
#include <linux/if_addr.h>
#include <linux/rtnetlink.h>
#include <sys/socket.h>
#include <net/if.h>
#include <arpa/inet.h>
#include <string.h>
#include <stdlib.h>

#include "addr.h"
#include "../../ip/addr.h"

#define NLMSG_TAIL(req)    ((struct rtattr *) (((void *) (&(req)->header)) + NLMSG_ALIGN((req)->header.nlmsg_len)))

struct network_netlink_ip_addr_req;

typedef struct network_netlink_ip_addr_req network_netlink_ip_addr_req_t;

struct network_netlink_ip_addr_req {
    struct nlmsghdr header;
    struct ifaddrmsg message;
    char buf[256];
};

static void network_netlink_ip_addr_req_addr_add(network_netlink_ip_addr_req_t * req, uint16_t type, uint8_t * inet, uint32_t len);
static void network_netlink_ip_addr_req_flags_add(network_netlink_ip_addr_req_t * req, uint16_t type, uint32_t value);

extern int64_t network_netlink_ip_addr_req(network_netlink_t * descriptor, uint8_t family, uint8_t * addr, uint32_t subnetmasklen, const char * dev, network_network_res_t callback) {
    network_netlink_ip_addr_req_t * req = network_netlink_request_pop(descriptor, network_netlink_ip_addr_req_t, true);

    req->header.nlmsg_len = NLMSG_LENGTH(sizeof(struct ifaddrmsg));
    req->header.nlmsg_flags = (NLM_F_REQUEST | NLM_F_CREATE | NLM_F_EXCL);
    req->header.nlmsg_type = RTM_NEWADDR;

    req->message.ifa_family = family;

    if(family == AF_INET) {
        uint8_t broadcast[4];
        network_netlink_ip_addr_req_addr_add(req, IFA_LOCAL, addr, 4);
        network_netlink_ip_addr_req_flags_add(req, IFA_FLAGS, 0);
        network_netlink_ip_addr_req_addr_add(req, IFA_ADDRESS, addr, 4);
        network_netlink_ip_addr_req_addr_add(req, IFA_BROADCAST, network_ip_addr_to_broadcast(addr, 4, subnetmasklen, broadcast), 4);

        req->message.ifa_prefixlen = subnetmasklen;
        req->message.ifa_scope = addr[0] == 127 ? RT_SCOPE_HOST : 0;
        req->message.ifa_index = if_nametoindex(dev);

        return network_netlink_req(descriptor, (struct nlmsghdr *) req, nil);
    } else {
#ifndef   RELEASE
        snorlaxdbg(true, false, "implement", "");
#endif // RELEASE
    }
    
    return fail;
}

static void network_netlink_ip_addr_req_addr_add(network_netlink_ip_addr_req_t * req, uint16_t type, uint8_t * addr, uint32_t len) {
    struct rtattr * attr = NLMSG_TAIL(req);

    int length = RTA_LENGTH(len);

    attr->rta_type = type;
    attr->rta_len = length;
    
    memcpy(RTA_DATA(attr), addr, len);

    req->header.nlmsg_len = NLMSG_ALIGN(req->header.nlmsg_len) + RTA_ALIGN(length);
}

static void network_netlink_ip_addr_req_flags_add(network_netlink_ip_addr_req_t * req, uint16_t type, uint32_t value) {
    struct rtattr * attr = NLMSG_TAIL(req);

    int length = RTA_LENGTH(sizeof(uint32_t));

    attr->rta_type = type;
    attr->rta_len = length;
    
    memcpy(RTA_DATA(attr), &value, sizeof(uint32_t));

    req->header.nlmsg_len = NLMSG_ALIGN(req->header.nlmsg_len) + RTA_ALIGN(length);
}
