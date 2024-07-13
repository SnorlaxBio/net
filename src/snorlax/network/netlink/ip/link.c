#include <linux/netlink.h>
#include <linux/rtnetlink.h>
#include <sys/socket.h>
// #include <linux/if.h>
#include <string.h>
#include <stdlib.h>
#include <net/if.h>

#include "link.h"

#define NLMSG_TAIL(req)    ((struct rtattr *) (((void *) (&(req)->header)) + NLMSG_ALIGN((req)->header.nlmsg_len)))

struct network_netlink_ip_link_req;

typedef struct network_netlink_ip_link_req network_netlink_ip_link_req_t;

struct network_netlink_ip_link_req {
    struct nlmsghdr header;
    struct ifinfomsg message;
    char buf[1024];
};

static void network_netlink_ip_link_req_if_index_add(network_netlink_ip_link_req_t * req, uint16_t type, uint32_t index);
static void network_netlink_ip_link_req_if_name_add(network_netlink_ip_link_req_t * req, uint16_t type, const char * name, uint32_t namelen);

extern network_netlink_req_t * network_netlink_ip_link_setup_req(network_netlink_t * descriptor, const char * dev, network_netlink_res_callback_t callback) {
    network_netlink_ip_link_req_t * req = (network_netlink_ip_link_req_t *) calloc(1, sizeof(network_netlink_ip_link_req_t));

    req->header.nlmsg_flags = NLM_F_REQUEST | NLM_F_ACK;
    req->header.nlmsg_len = NLMSG_LENGTH(sizeof(struct ifinfomsg));
    req->header.nlmsg_type = RTM_NEWLINK;

    req->message.ifi_family = AF_UNSPEC;
    req->message.ifi_change = IFF_UP;
    req->message.ifi_flags = IFF_UP;
    req->message.ifi_index = if_nametoindex(dev);

    network_netlink_ip_link_req_if_index_add(req, IFLA_NEW_IFINDEX, req->message.ifi_index);
    network_netlink_ip_link_req_if_name_add(req, IFLA_IFNAME, dev, strlen(dev) + 1);

    return network_netlink_req(descriptor, (struct nlmsghdr *) req, callback);
}

static void network_netlink_ip_link_req_if_index_add(network_netlink_ip_link_req_t * req, uint16_t type, uint32_t index) {
    struct rtattr * attr = NLMSG_TAIL(req);

    int length = RTA_LENGTH(sizeof(uint32_t));

    attr->rta_type = type;
    attr->rta_len = length;
    
    memcpy(RTA_DATA(attr), &index, sizeof(uint32_t));

    req->header.nlmsg_len = NLMSG_ALIGN(req->header.nlmsg_len) + RTA_ALIGN(length);
}

static void network_netlink_ip_link_req_if_name_add(network_netlink_ip_link_req_t * req, uint16_t type, const char * name, uint32_t namelen) {
    struct rtattr * attr = NLMSG_TAIL(req);

    int length = RTA_LENGTH(namelen);

    attr->rta_type = type;
    attr->rta_len = length;
    
    memcpy(RTA_DATA(attr), name, namelen);

    req->header.nlmsg_len = NLMSG_ALIGN(req->header.nlmsg_len) + RTA_ALIGN(length);
}

extern void network_netlink_ip_link_setup_res(void * request, uint64_t requestlen, void * response, uint64_t responselen) {
    printf("network_netlink_ip_link_setup_res\n");
}