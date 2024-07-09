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

#include <snorlax.h>
#include <snorlax/descriptor.h>
// #include <linux/netlink.h>

struct network_netlink;
struct network_netlink_func;

typedef struct network_netlink network_netlink_t;
typedef struct network_netlink_func network_netlink_func_t;

struct network_netlink {
    network_netlink_func_t * func;
    sync_t * sync;
    descriptor_buffer_t buffer;
    descriptor_exception_t exception;
    int32_t value;
    uint32_t status;
    uint32_t subscriptions;
    uint32_t seq;
};

typedef void (*network_netlink_res_t)(void *);

struct network_netlink_func {
    network_netlink_t * (*rem)(___notnull network_netlink_t *);

    int32_t (*open)(___notnull network_netlink_t *);
    int64_t (*read)(___notnull network_netlink_t *);
    int64_t (*write)(___notnull network_netlink_t *);
    int32_t (*close)(___notnull network_netlink_t *);
    int32_t (*check)(___notnull network_netlink_t *, uint32_t);

    int32_t (*req)(___notnull network_netlink_t *, void *, network_netlink_res_t);
};

extern network_netlink_t * network_netlink_gen(uint32_t subscriptions);

extern network_netlink_t * network_netlink_get(void);

#define network_netlink_rem(descriptor)                             ((descriptor)->func->rem(descriptor))
#define network_netlink_open(descriptor)                            ((descriptor)->func->open(descriptor))
#define network_netlink_read(descriptor)                            ((descriptor)->func->read(descriptor))
#define network_netlink_write(descriptor)                           ((descriptor)->func->write(descriptor))
#define network_netlink_close(descriptor)                           ((descriptor)->func->close(descriptor))
#define network_netlink_check(descriptor, state)                    ((descriptor)->func->check(descriptor, state))
#define network_netlink_request_pop(descriptor, request, clear)     ((request *) buffer_pop((descriptor)->buffer.out, sizeof(request), clear))
#define network_netlink_req(descriptor, request, callback)          ((descriptor)->func->req(descriptor, request, callback))

#endif // __SNORLAX__NETWORK_NETLINK__H__
