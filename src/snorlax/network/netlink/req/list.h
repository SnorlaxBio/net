/**
 * @file        snorlax/network/netlink/req/list.h
 * @brief
 * @details
 * 
 * @author      snorlax <ceo@snorlax.bio>
 * @since       July 11, 2024
 */

#ifndef   __SNORLAX__NETWORK_NETLINK_REQ_LIST__H__
#define   __SNORLAX__NETWORK_NETLINK_REQ_LIST__H__

#include <snorlax.h>
#include <snorlax/network/netlink/req.h>

struct network_netlink_req_list;
struct network_netlink_req_list_func;

typedef struct network_netlink_req_list network_netlink_req_list_t;
typedef struct network_netlink_req_list_func network_netlink_req_list_func_t;

struct network_netlink_req_list {
    network_netlink_req_list_func_t * func;
    sync_t * sync;
    uint64_t size;
    network_netlink_req_t * head;
    network_netlink_req_t * tail;

    network_netlink_req_t * last;
};

struct network_netlink_req_list_func {
    network_netlink_req_list_t * (*rem)(network_netlink_req_list_t *);
    network_netlink_req_t * (*add)(network_netlink_req_list_t *, network_netlink_req_t *);
    network_netlink_req_t * (*del)(network_netlink_req_list_t *, network_netlink_req_t *);
};

extern network_netlink_req_list_t * network_netlink_req_list_gen(void);

#define network_netlink_req_list_rem(list)          ((list)->func->rem(list))
#define network_netlink_req_list_add(list, req)     ((list)->func->add(list, req))
#define network_netlink_req_list_del(list, req)     ((list)->func->del(list, req))

#endif // __SNORLAX__NETWORK_NETLINK_REQ_LIST__H__
