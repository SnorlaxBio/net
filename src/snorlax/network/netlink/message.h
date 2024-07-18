/**
 * @file        snorlax/network/netlink/message.h
 * @brief
 * @details
 * 
 * @author      snorlax <ceo@snorlax.bio>
 * @since       July 15, 2024
 */

#ifndef   __SNORLAX__NETWORK_NETLINK_MESSAGE__H__
#define   __SNORLAX__NETWORK_NETLINK_MESSAGE__H__

#include <linux/netlink.h>

#include <snorlax.h>
#include <snorlax/buffer/list.h>

struct network_netlink_message;
struct network_netlink_message_func;

typedef struct network_netlink_message network_netlink_message_t;
typedef struct network_netlink_message_func network_netlink_message_func_t;

struct network_netlink_message {
    network_netlink_message_func_t * func;
    sync_t * sync;
    buffer_list_t * collection;
    network_netlink_message_t * prev;
    network_netlink_message_t * next;

    uint32_t status;
    struct nlmsghdr * message;
};

struct network_netlink_message_func {
    network_netlink_message_t * (*rem)(network_netlink_message_t *);
};

extern network_netlink_message_t * network_netlink_message_gen(struct nlmsghdr * nlmsg);

extern struct nlmsghdr * network_netlink_message_ipaddr_add_gen(uint8_t family, uint8_t * inet, uint32_t subnetmasklen, const char * dev);
extern struct nlmsghdr * network_netlink_message_iplink_setup_gen(const char * dev);
extern struct nlmsghdr * network_netlink_message_iproute_prepend_gen(uint8_t * addr, uint32_t subnetmasklen, uint8_t * next);
extern struct nlmsghdr * network_netlink_message_iproute_get_gen(void);
extern struct nlmsghdr * network_netlink_message_iprule_add_gen(uint32_t mark, uint8_t table);

extern void network_netlink_message_rtattr_object_add(struct nlmsghdr * req, uint16_t type, const uint8_t * data, uint32_t len);
extern void network_netlink_message_rtattr_uint32_add(struct nlmsghdr * req, uint16_t type, uint32_t value);
extern void network_netlink_message_rtattr_int32_add(struct nlmsghdr * req, uint16_t type, int32_t value);

#define network_netlnk_message_tail(req)    ((struct rtattr *) (((void *) (req)) + NLMSG_ALIGN((req)->nlmsg_len)))

#endif // __SNORLAX__NETWORK_NETLINK_MESSAGE__H__
