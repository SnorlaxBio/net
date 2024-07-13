/**
 * @file        snorlax/network/netlink/req.h
 * @brief
 * @details
 * 
 * @author      snorlax <ceo@snorlax.bio>
 * @since       July 11, 2024
 */

#ifndef   __SNORLAX__NETWORK_NETLINK_REQ__H__
#define   __SNORLAX__NETWORK_NETLINK_REQ__H__

#include <snorlax.h>

struct network_netlink_req;
struct network_netlink_req_func;

struct network_netlink_req_list;

typedef struct network_netlink_req network_netlink_req_t;
typedef struct network_netlink_req_func network_netlink_req_func_t;

typedef struct network_netlink_req_list network_netlink_req_list_t;

typedef void (*network_netlink_res_callback_t)(void *, uint64_t, void *, uint64_t);

struct network_netlink_req {
    network_netlink_req_func_t * func;
    sync_t * sync;
    uint64_t size;
    network_netlink_req_list_t * collection;
    network_netlink_req_t * prev;
    network_netlink_req_t * next;

    uint64_t start;
    uint64_t end;
    uint32_t sequence;
    void * message;
    uint64_t messagelen;
    network_netlink_res_callback_t callback;
};

struct network_netlink_req_func {
    network_netlink_req_t * (*rem)(network_netlink_req_t *);
};

extern network_netlink_req_t * network_netlink_req_gen(network_netlink_req_list_t * collection, uint32_t sequence, void * message, uint64_t messagelen, network_netlink_res_callback_t callback);

#define network_netlink_req_rem(req)        ((req)->func->rem(req))

#endif // __SNORLAX__NETWORK_NETLINK_REQ__H__
