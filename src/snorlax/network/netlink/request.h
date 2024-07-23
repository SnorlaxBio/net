/**
 * @file        snorlax/network/netlink/request.h
 * @brief
 * @details
 * 
 * @author      snorlax <ceo@snorlax.bio>
 * @since       July 22, 2024
 */

#ifndef   __SNORLAX__NETWORK_NETLINK_REQUEST__H__
#define   __SNORLAX__NETWORK_NETLINK_REQUEST__H__

#include <linux/netlink.h>

#include <snorlax.h>
#include <snorlax/buffer/list.h>

struct network_netlink_request;
struct network_netlink_request_func;

typedef struct network_netlink_request network_netlink_request_t;
typedef struct network_netlink_request_func network_netlink_request_func_t;

typedef void (*network_netlink_request_callback_t)(struct nlmsghdr *, uint32_t, struct nlmsghdr *);

struct network_netlink_request {
    network_netlink_request_func_t * func;
    sync_t * sync;
    buffer_list_t * collection;
    network_netlink_request_t * prev;
    network_netlink_request_t * next;
    uint64_t position;
    uint64_t size;
    uint64_t capacity;
    struct nlmsghdr * message;
    uint32_t status;
    uint64_t done;
    network_netlink_request_callback_t callback;
};

struct network_netlink_request_func {
    network_netlink_request_t * (*rem)(network_netlink_request_t *);
    void * (*front)(network_netlink_request_t *);
    void * (*back)(network_netlink_request_t *);
    int32_t (*shrink)(network_netlink_request_t *);
    uint64_t (*length)(network_netlink_request_t *);
    uint64_t (*remain)(network_netlink_request_t *);
    uint64_t (*position_get)(network_netlink_request_t *);
    void (*position_set)(network_netlink_request_t *, uint64_t);
    uint64_t (*size_get)(network_netlink_request_t *);
    void (*size_set)(network_netlink_request_t *, uint64_t);
    uint64_t (*capacity_get)(network_netlink_request_t *);
    void (*capacity_set)(network_netlink_request_t *, uint64_t);
    void (*clear)(network_netlink_request_t *);

    struct nlmsghdr * (*nlmsghdr_get)(network_netlink_request_t *);

    uint64_t (*done_get)(network_netlink_request_t *);
    void (*done_set)(network_netlink_request_t *, uint64_t);
};

extern network_netlink_request_t * network_netlink_request_gen(buffer_list_t * buffer, struct nlmsghdr * nlmsg, uint64_t n, network_netlink_request_callback_t callback);

#define network_netlink_request_rem(message)                    ((message)->func->rem(message))
#define network_netlink_request_front(message)                  ((message)->func->front(message))
#define network_netlink_request_back(message)                   ((message)->func->back(message))
#define network_netlink_request_shrink(message)                 ((message)->func->shrink(message))
#define network_netlink_request_length(message)                 ((message) ? (message)->func->length(message) : 0)
#define network_netlink_request_remain(message)                 ((message) ? (message)->func->remain(message) : 0)
#define network_netlink_request_position_get(message)           ((message)->func->position_get(message))
#define network_netlink_request_position_set(message, v)        ((message)->func->position_set(message, v))
#define network_netlink_request_size_get(message)               ((message)->func->size_get(message))
#define network_netlink_request_size_set(message, v)            ((message)->func->size_set(message, v))
#define network_netlink_request_capacity_get(message)           ((message)->func->capacity_get(message))
#define network_netlink_request_capacity_set(message, v)        ((message)->func->capacity_set(message, v))
#define network_netlink_request_clear(message)                  ((message)->func->clear(message))
#define network_netlink_request_nlmsghdr_get(message)           ((message) ? (message)->func->nlmsghdr_get(message) : nil)
#define network_netlink_request_done_get(message)               ((message)->func->done_get(message))
#define network_netlink_request_done_set(message, v)            ((message)->func->done_set(message, v))

#endif // __SNORLAX__NETWORK_NETLINK_REQUEST__H__