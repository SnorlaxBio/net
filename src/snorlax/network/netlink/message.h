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
    uint64_t position;
    uint64_t size;
    uint64_t capacity;
    struct nlmsghdr * message;
    uint32_t status;
};

struct network_netlink_message_func {
    network_netlink_message_t * (*rem)(network_netlink_message_t *);
    void * (*front)(network_netlink_message_t *);
    void * (*back)(network_netlink_message_t *);
    uint64_t (*length)(network_netlink_message_t *);
    uint64_t (*remain)(network_netlink_message_t *);
    uint64_t (*position_get)(network_netlink_message_t *);
    void (*position_set)(network_netlink_message_t *, uint64_t);
    uint64_t (*size_get)(network_netlink_message_t *);
    void (*size_set)(network_netlink_message_t *, uint64_t);
    uint64_t (*capacity_get)(network_netlink_message_t *);
    void (*capacity_set)(network_netlink_message_t *, uint64_t);
    void (*clear)(network_netlink_message_t *);
};

extern network_netlink_message_t * network_netlink_message_gen(buffer_list_t * buffer, struct nlmsghdr * nlmsg, uint64_t n);

#define network_netlink_message_rem(message)                    ((message)->func->rem(message))
#define network_netlink_message_front(message)                  ((message)->func->front(message))
#define network_netlink_message_back(message)                   ((message)->func->back(message))
#define network_netlink_message_length(message)                 ((message)->func->length(message))
#define network_netlink_message_remain(message)                 ((message)->func->remain(message))
#define network_netlink_message_position_get(message)           ((message)->func->position_get(message))
#define network_netlink_message_position_set(message, v)        ((message)->func->position_set(message, v))
#define network_netlink_message_size_get(message)               ((message)->func->size_get(message))
#define network_netlink_message_size_set(message, v)            ((message)->func->size_set(message, v))
#define network_netlink_message_capacity_get(message)           ((message)->func->capacity_get(message))
#define network_netlink_message_capacity_set(message, v)        ((message)->func->capacity_set(message, v))
#define network_netlink_message_clear(message)                  ((message)->func->clear(message))

extern struct nlmsghdr * network_netlink_message_ipaddr_add_gen(uint8_t family, uint8_t * inet, uint32_t subnetmasklen, const char * dev);
extern struct nlmsghdr * network_netlink_message_iplink_setup_gen(const char * dev);
extern struct nlmsghdr * network_netlink_message_iproute_prepend_gen(uint8_t * addr, uint32_t subnetmasklen, uint8_t * next, uint8_t table);
extern struct nlmsghdr * network_netlink_message_iproute_get_gen(void);
extern struct nlmsghdr * network_netlink_message_iprule_add_gen(uint32_t mark, uint32_t priority, uint8_t table);

extern void network_netlink_message_rtattr_object_add(struct nlmsghdr * req, uint16_t type, const uint8_t * data, uint32_t len);
extern void network_netlink_message_rtattr_uint32_add(struct nlmsghdr * req, uint16_t type, uint32_t value);
extern void network_netlink_message_rtattr_int32_add(struct nlmsghdr * req, uint16_t type, int32_t value);

#define network_netlnk_message_tail(req)    ((struct rtattr *) (((void *) (req)) + NLMSG_ALIGN((req)->nlmsg_len)))

#endif // __SNORLAX__NETWORK_NETLINK_MESSAGE__H__
