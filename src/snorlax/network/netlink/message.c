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

typedef network_netlink_message_t * (*network_netlink_message_func_rem_t)(network_netlink_message_t *);
typedef void * (*network_netlink_message_func_front_t)(network_netlink_message_t *);
typedef void * (*network_netlink_message_func_back_t)(network_netlink_message_t *);
typedef int32_t (*network_netlink_message_func_shrink_t)(network_netlink_message_t *);
typedef uint64_t (*network_netlink_message_func_length_t)(network_netlink_message_t *);
typedef uint64_t (*network_netlink_message_func_remain_t)(network_netlink_message_t *);
typedef uint64_t (*network_netlink_message_func_position_get_t)(network_netlink_message_t *);
typedef void (*network_netlink_message_func_position_set_t)(network_netlink_message_t *, uint64_t);
typedef uint64_t (*network_netlink_message_func_size_get_t)(network_netlink_message_t *);
typedef void (*network_netlink_message_func_size_set_t)(network_netlink_message_t *, uint64_t);
typedef uint64_t (*network_netlink_message_func_capacity_get_t)(network_netlink_message_t *);
typedef void (*network_netlink_message_func_capacity_set_t)(network_netlink_message_t *, uint64_t);
typedef void (*network_netlink_message_func_clear_t)(network_netlink_message_t *);

static uint8_t * network_ipaddr_to_broadcast(uint8_t * in, uint32_t len, uint32_t subnetmasklen, uint32_t * out);

static struct nlmsghdr * network_netlink_message_func_nlmsghdr_get(network_netlink_message_t * message);

static network_netlink_message_func_t func = {
    (network_netlink_message_func_rem_t) buffer_list_node_func_rem,
    (network_netlink_message_func_front_t) buffer_list_node_func_front,
    (network_netlink_message_func_back_t) buffer_list_node_func_back,
    (network_netlink_message_func_shrink_t) buffer_list_node_func_shrink,
    (network_netlink_message_func_length_t) buffer_list_node_func_length,
    (network_netlink_message_func_remain_t) buffer_list_node_func_remain,
    (network_netlink_message_func_position_get_t) buffer_list_node_func_position_get,
    (network_netlink_message_func_position_set_t) buffer_list_node_func_position_set,
    (network_netlink_message_func_size_get_t) buffer_list_node_func_size_get,
    (network_netlink_message_func_size_set_t) buffer_list_node_func_size_set,
    (network_netlink_message_func_capacity_get_t) buffer_list_node_func_capacity_get,
    (network_netlink_message_func_capacity_set_t) buffer_list_node_func_capacity_set,
    (network_netlink_message_func_clear_t) buffer_list_node_func_clear,
    network_netlink_message_func_nlmsghdr_get
};

extern network_netlink_message_t * network_netlink_message_gen(buffer_list_t * buffer, struct nlmsghdr * nlmsg, uint64_t n) {
    network_netlink_message_t * node = (network_netlink_message_t *) calloc(1, sizeof(network_netlink_message_t));

    node->func = address_of(func);

    buffer_list_add(buffer, (buffer_list_node_t *) node);

    uint64_t page = buffer->page ? buffer->page : 1;

    if(nlmsg) {
        node->capacity = nlmsg->nlmsg_len;
        node->size = nlmsg->nlmsg_len;
        node->message = nlmsg;
    } else if(n > 0) {
        node->capacity = n;
        node->message = (struct nlmsghdr *) malloc(n);
    }

    return node;
}

static struct nlmsghdr * network_netlink_message_func_nlmsghdr_get(network_netlink_message_t * node) {
#ifndef   RELEASE
    snorlaxdbg(node == nil, false, "critical", "");
#endif // RELEASE

    uint8_t * mem = (uint8_t *) node->message;

    return node->size <= node->position ? nil : (struct nlmsghdr *) &mem[node->position];
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

extern struct nlmsghdr * network_netlink_message_iproute_prepend_gen(uint8_t * addr, uint32_t subnetmasklen, uint8_t * next, uint8_t table) {
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
    req->message.rtm_table = table;

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
    if(mark != (uint32_t) (-1)) {
        network_netlink_message_rtattr_uint32_add((struct nlmsghdr *) req, FRA_FWMARK, mark);
    }
    network_netlink_message_rtattr_uint32_add((struct nlmsghdr *) req, FRA_PRIORITY, priority);

    return (struct nlmsghdr *) req;
}

static uint8_t * network_ipaddr_to_broadcast(uint8_t * in, uint32_t len, uint32_t subnetmasklen, uint32_t * out) {
    uint32_t subnetmask = 0;
    for(int32_t i = 0; i < subnetmasklen; i++) {
        subnetmask |= (1 << i);
    }

    *out = ((*((uint32_t *) in)) & subnetmask) | (~subnetmask);

    return (uint8_t *) out;
}
