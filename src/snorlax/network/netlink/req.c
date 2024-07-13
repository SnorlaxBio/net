#include "req.h"

#include "req/list.h"

static network_netlink_req_t * network_netlink_req_func_rem(network_netlink_req_t * req);

typedef network_netlink_req_t * (*network_netlink_req_func_rem_t)(network_netlink_req_t *);

static network_netlink_req_func_t func = {
    (network_netlink_req_func_rem_t) network_netlink_req_func_rem
};

extern network_netlink_req_t * network_netlink_req_gen(network_netlink_req_list_t * collection, uint32_t sequence, void * message, uint64_t messagelen, network_netlink_res_callback_t callback) {
#ifndef   RELEASE
    snorlaxdbg(collection == nil, false, "critical", "");
#endif // RELEASE
    network_netlink_req_t * req = (network_netlink_req_t *) calloc(1, sizeof(network_netlink_req_t));

    req->func = address_of(func);

    network_netlink_req_list_add(collection, req);

    req->sequence = sequence;
    req->message = message;
    req->messagelen = messagelen;
    req->callback = callback;

    return req;
}

static network_netlink_req_t * network_netlink_req_func_rem(network_netlink_req_t * req) {
#ifndef   RELEASE
    snorlaxdbg(req == nil, false, "critical", "");
#endif // RELEASE

    if(req->collection) network_netlink_req_list_del(req->collection, req);

    req->message = memory_rem(req->message);

    req = memory_rem(req);

    return nil;
}