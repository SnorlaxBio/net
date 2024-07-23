/**
 * @file        snorlax/network/netlink.h
 * @brief
 * @details
 * 
 * @author      snorlax <ceo@snorlax.bio>
 * @since       July 7, 2024
 */

#ifndef   __SNORLAX__NETWORK_NETLINK__H__
#define   __SNORLAX__NETWORK_NETLINK__H__

#include <linux/netlink.h>

#include <snorlax.h>
#include <snorlax/buffer/list.h>
#include <snorlax/descriptor.h>
#include <snorlax/socket.h>
#include <snorlax/network/netlink/message.h>
#include <snorlax/network/netlink/request.h>

struct network_netlink;
struct network_netlink_func;
struct network_netlink_req;

typedef struct network_netlink network_netlink_t;
typedef struct network_netlink_func network_netlink_func_t;
typedef struct network_netlink_req network_netlink_req_t;

struct network_netlink_buffer;
typedef struct network_netlink_buffer network_netlink_buffer_t;

typedef void (*network_netlink_request_callback_t)(struct nlmsghdr *, uint32_t, struct nlmsghdr *);

struct network_netlink_buffer {
    buffer_list_t * in;
    buffer_list_t * out;
};

struct network_netlink {
    network_netlink_func_t * func;
    sync_t * sync;
    network_netlink_buffer_t buffer;
    descriptor_exception_t exception;
    int32_t value;
    uint32_t status;

    uint32_t subscriptions;
    uint32_t seq;
};

struct network_netlink_func {
    network_netlink_t * (*rem)(___notnull network_netlink_t *);

    int32_t (*open)(___notnull network_netlink_t *);
    int64_t (*read)(___notnull network_netlink_t *);
    int64_t (*write)(___notnull network_netlink_t *);
    int32_t (*close)(___notnull network_netlink_t *);
    int32_t (*check)(___notnull network_netlink_t *, uint32_t);

    network_netlink_request_t * (*req)(___notnull network_netlink_t *, ___notnull socket_event_subscription_t *, struct nlmsghdr *, network_netlink_request_callback_t);
};

#define network_netlink_table_main_mark             1
#define network_netlink_table_main_priority         100
#define network_netlink_table_main_id               254

#define network_netlink_table_tun_mark              (-1)
#define network_netlink_table_tun_priority          101
#define network_netlink_table_tun_id                100

extern network_netlink_t * network_netlink_gen(uint32_t subscriptions);

extern network_netlink_t * network_netlink_get(void);

#define network_netlink_rem(descriptor)                                     ((descriptor)->func->rem(descriptor))
#define network_netlink_open(descriptor)                                    ((descriptor)->func->open(descriptor))
#define network_netlink_read(descriptor)                                    ((descriptor)->func->read(descriptor))
#define network_netlink_write(descriptor)                                   ((descriptor)->func->write(descriptor))
#define network_netlink_close(descriptor)                                   ((descriptor)->func->close(descriptor))
#define network_netlink_check(descriptor, state)                            ((descriptor)->func->check(descriptor, state))
#define network_netlink_req(descriptor, subscription, message, callback)    ((descriptor)->func->req(descriptor, subscription, message, callback))

#define network_netlink_message_ok(message, len)                            NLMSG_OK(message, len)
#define network_netlink_message_next(message, len)                          NLMSG_NEXT(message, len)

#define netlink_protocol_data_get(type, address, len)                       ((type)(((void *) address) + NLMSG_ALIGN(len)))
#define netlink_protocol_data_end(header)                                   ((((void *)(header)) + NLMSG_ALIGN(header->nlmsg_len)))
#define netlink_protocol_attr_next(attr)                                    ((struct rtattr *)(((void *)(attr))  + RTA_ALIGN((attr)->rta_len)))

extern void netlink_protocol_debug(FILE * stream, void * data);

#endif // __SNORLAX__NETWORK_NETLINK__H__
