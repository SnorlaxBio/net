#include <linux/netlink.h>
#include <linux/if_addr.h>
#include <linux/rtnetlink.h>
#include <sys/socket.h>
#include <net/if.h>
#include <arpa/inet.h>
#include <string.h>
#include <stdlib.h>
#include <linux/fib_rules.h>

#include "message.h"

static uint8_t * network_ipaddr_to_broadcast(uint8_t * in, uint32_t len, uint32_t subnetmasklen, uint32_t * out);

static uint8_t * network_ipaddr_to_broadcast(uint8_t * in, uint32_t len, uint32_t subnetmasklen, uint32_t * out) {
    uint32_t subnetmask = 0;
    for(int32_t i = 0; i < subnetmasklen; i++) {
        subnetmask |= (1 << i);
    }

    *out = ((*((uint32_t *) in)) & subnetmask) | (~subnetmask);

    return (uint8_t *) out;
}

static network_netlink_message_t * network_netlink_message_func_rem(network_netlink_message_t * message);

static network_netlink_message_func_t func = {
    network_netlink_message_func_rem
};

extern network_netlink_message_t * network_netlink_message_gen(struct nlmsghdr * nlmsg) {
    network_netlink_message_t * message = (network_netlink_message_t *) calloc(1, sizeof(network_netlink_message_t));

    message->func = address_of(func);

    if(nlmsg) message->message = memory_dup(nlmsg, nlmsg->nlmsg_len);

    return message;
}

static network_netlink_message_t * network_netlink_message_func_rem(network_netlink_message_t * message) {
#ifndef   RELEASE
    snorlaxdbg(message == nil, false, "critical", "");
#endif // RELEASE

    if(message->collection) buffer_list_del(message->collection, (buffer_list_node_t *) message);

    message->message = memory_rem(message->message);
    message->sync = sync_rem(message->sync);

    free(message);

    return nil;
}

extern void network_netlink_message_rtattr_object_add(struct nlmsghdr * req, uint16_t type, const uint8_t * data, uint32_t len) {
    struct rtattr * attr = network_netlnk_message_tail(req);

    int length = RTA_LENGTH(len);

    attr->rta_type = type;
    attr->rta_len = length;

    memcpy(RTA_DATA(attr), data, length);

    req->nlmsg_len = NLMSG_ALIGN(req->nlmsg_len) + RTA_ALIGN(length);
}

extern void network_netlink_message_rtattr_int32_add(struct nlmsghdr * req, uint16_t type, int32_t value) {
    struct rtattr * attr = network_netlnk_message_tail(req);

    int length = RTA_LENGTH(sizeof(int32_t));

    attr->rta_type = type;
    attr->rta_len = length;
    
    memcpy(RTA_DATA(attr), &value, sizeof(int32_t));

    req->nlmsg_len = NLMSG_ALIGN(req->nlmsg_len) + RTA_ALIGN(length);
}

extern void network_netlink_message_rtattr_uint32_add(struct nlmsghdr * req, uint16_t type, uint32_t value) {
    struct rtattr * attr = network_netlnk_message_tail(req);

    int length = RTA_LENGTH(sizeof(uint32_t));

    attr->rta_type = type;
    attr->rta_len = length;
    
    memcpy(RTA_DATA(attr), &value, sizeof(uint32_t));

    req->nlmsg_len = NLMSG_ALIGN(req->nlmsg_len) + RTA_ALIGN(length);
}

struct network_netlink_message_ipaddr_req {
    struct nlmsghdr header;
    struct ifaddrmsg message;
    char buf[256];
};

typedef struct network_netlink_message_ipaddr_req network_netlink_message_ipaddr_req_t;

extern struct nlmsghdr * network_netlink_message_ipaddr_add_gen(uint8_t family, uint8_t * addr, uint32_t subnetmasklen, const char * dev) {
    if(family == AF_INET) {
        network_netlink_message_ipaddr_req_t * req = (network_netlink_message_ipaddr_req_t *) calloc(1, sizeof(network_netlink_message_ipaddr_req_t));

        req->header.nlmsg_len = NLMSG_LENGTH(sizeof(struct ifaddrmsg));
        req->header.nlmsg_flags = (NLM_F_REQUEST | NLM_F_CREATE | NLM_F_EXCL | NLM_F_ACK);
        req->header.nlmsg_type = RTM_NEWADDR;

        req->message.ifa_family = family;

        req->message.ifa_prefixlen = subnetmasklen;
        req->message.ifa_scope = addr[0] == 127 ? RT_SCOPE_HOST : 0;
        req->message.ifa_index = if_nametoindex(dev);

        uint8_t broadcast[4];

        network_netlink_message_rtattr_object_add((struct nlmsghdr *) req, IFA_LOCAL, addr, 4);
        network_netlink_message_rtattr_uint32_add((struct nlmsghdr *) req, IFA_FLAGS, 0);
        network_netlink_message_rtattr_object_add((struct nlmsghdr *) req, IFA_ADDRESS, addr, 4);
        network_netlink_message_rtattr_object_add((struct nlmsghdr *) req, IFA_BROADCAST, network_ipaddr_to_broadcast(addr, 4, subnetmasklen, (uint32_t *) broadcast), 4);

        return (struct nlmsghdr *) req;
    }

    return nil;
}

struct network_netlink_message_iplink_req {
    struct nlmsghdr header;
    struct ifinfomsg message;
    char buf[1024];
};

typedef struct network_netlink_message_iplink_req network_netlink_message_iplink_req_t;

extern struct nlmsghdr * network_netlink_message_iplink_setup_gen(const char * dev) {
    network_netlink_message_iplink_req_t * req = (network_netlink_message_iplink_req_t *) calloc(1, sizeof(network_netlink_message_iplink_req_t));

    req->header.nlmsg_flags = NLM_F_REQUEST | NLM_F_ACK;
    req->header.nlmsg_len = NLMSG_LENGTH(sizeof(struct ifinfomsg));
    req->header.nlmsg_type = RTM_NEWLINK;

    req->message.ifi_family = AF_UNSPEC;
    req->message.ifi_change = IFF_UP;
    req->message.ifi_flags = IFF_UP;
    req->message.ifi_index = if_nametoindex(dev);

    network_netlink_message_rtattr_int32_add((struct nlmsghdr *) req, IFLA_NEW_IFINDEX, req->message.ifi_index);
    network_netlink_message_rtattr_object_add((struct nlmsghdr *) req, IFLA_IFNAME, dev, strlen(dev) + 1);

    return (struct nlmsghdr *) req;
}

struct network_netlink_message_iproute_req {
    struct nlmsghdr header;
    struct rtmsg message;
    char buf[4096];
};

typedef struct network_netlink_message_iproute_req network_netlink_message_iproute_req_t;

extern struct nlmsghdr * network_netlink_message_iproute_get_gen(void) {
    network_netlink_message_iproute_req_t * req = (network_netlink_message_iproute_req_t *) calloc(1, sizeof(network_netlink_message_iproute_req_t));

    req->header.nlmsg_flags = NLM_F_REQUEST | NLM_F_DUMP | NLM_F_ACK;
    req->header.nlmsg_len = NLMSG_LENGTH(sizeof(struct rtmsg));
    req->header.nlmsg_type = RTM_GETROUTE;

    req->message.rtm_family = AF_INET;

    return (struct nlmsghdr *) req;
}

extern struct nlmsghdr * network_netlink_message_iproute_prepend_gen(uint8_t * addr, uint32_t subnetmasklen, uint8_t * next) {
    network_netlink_message_iproute_req_t * req = (network_netlink_message_iproute_req_t *) calloc(1, sizeof(network_netlink_message_iproute_req_t));

    req->header.nlmsg_flags = NLM_F_REQUEST | NLM_F_CREATE | NLM_F_ACK;
    req->header.nlmsg_len = NLMSG_LENGTH(sizeof(struct rtmsg));
    req->header.nlmsg_type = RTM_NEWROUTE;

    req->message.rtm_family = AF_INET;
    req->message.rtm_table = RT_TABLE_MAIN;
    req->message.rtm_scope = RT_SCOPE_UNIVERSE;
    req->message.rtm_protocol = RTPROT_BOOT;
    req->message.rtm_type = RTN_UNICAST;
    req->message.rtm_dst_len = strcmp(addr, "default") == 0 ? -2 : subnetmasklen;

    network_netlink_message_rtattr_object_add((struct nlmsghdr *) req, RTA_DST, addr, 4);
    network_netlink_message_rtattr_object_add((struct nlmsghdr *) req, RTA_GATEWAY, next, 4);

    return (struct nlmsghdr *) req;
}

struct network_netlink_message_iprule_req {
    struct nlmsghdr	header;
    struct fib_rule_hdr	message;
    char buf[1024];
};

typedef struct network_netlink_message_iprule_req network_netlink_message_iprule_req_t;

extern struct nlmsghdr * network_netlink_message_iprule_add_gen(uint32_t mark, uint32_t priority, uint8_t table) {
    network_netlink_message_iprule_req_t * req = (network_netlink_message_iprule_req_t *) calloc(1, sizeof(network_netlink_message_iprule_req_t));

    req->header.nlmsg_flags = NLM_F_REQUEST | NLM_F_CREATE | NLM_F_EXCL | NLM_F_ACK;
    req->header.nlmsg_len = NLMSG_LENGTH(sizeof(struct rtmsg));
    req->header.nlmsg_type = RTM_NEWRULE;

    req->message.family = AF_INET;
    req->message.action = FR_ACT_TO_TBL;
    req->message.src_len = 0;
    req->message.table = table;

    uint8_t addr[4] = { 0, 0, 0, 0 };
    network_netlink_message_rtattr_object_add((struct nlmsghdr *) req, FRA_SRC, addr, 4);
    network_netlink_message_rtattr_uint32_add((struct nlmsghdr *) req, FRA_FWMARK, mark);
    network_netlink_message_rtattr_uint32_add((struct nlmsghdr *) req, FRA_PRIORITY, priority);

    return (struct nlmsghdr *) req;
}