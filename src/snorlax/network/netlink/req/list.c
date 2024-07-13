#include "list.h"

static network_netlink_req_list_t * network_netlink_req_list_func_rem(network_netlink_req_list_t * collection);
static network_netlink_req_t * network_netlink_req_list_func_add(network_netlink_req_list_t * collection, network_netlink_req_t * req);
static network_netlink_req_t * network_netlink_req_list_func_del(network_netlink_req_list_t * collection, network_netlink_req_t * req);

static network_netlink_req_list_func_t func = {
    network_netlink_req_list_func_rem,
    network_netlink_req_list_func_add,
    network_netlink_req_list_func_del
};

extern network_netlink_req_list_t * network_netlink_req_list_gen(void) {
    network_netlink_req_list_t * collection = (network_netlink_req_list_t *) calloc(1, sizeof(network_netlink_req_list_t));

    collection->func = address_of(func);

    return collection;
}

static network_netlink_req_list_t * network_netlink_req_list_func_rem(network_netlink_req_list_t * collection) {
    network_netlink_req_t * node = collection->head;

    while(collection->head) {
        collection->head = node->next;

        node->prev = nil;
        node->next = nil;
        node->collection = nil;

        if(collection->last == node) collection->last = nil;

        network_netlink_req_rem(node);

        node = collection->head;
    }

    free(collection);

    return nil;
}

static network_netlink_req_t * network_netlink_req_list_func_add(network_netlink_req_list_t * collection, network_netlink_req_t * req) {
#ifndef   RELEASE
    snorlaxdbg(collection == nil, false, "critical", "");
    snorlaxdbg(req == nil, false, "critical", "");
    snorlaxdbg(req->collection, false, "critical", "");
#endif // RELEASE

    if(collection->tail) {
        collection->tail->next = req;
        req->prev = collection->tail;
    } else {
        collection->head = req;
    }

    collection->tail = req;
    collection->size = collection->size + 1;
    req->collection = collection;

    return req;
}

static network_netlink_req_t * network_netlink_req_list_func_del(network_netlink_req_list_t * collection, network_netlink_req_t * req) {
#ifndef   RELEASE
    snorlaxdbg(collection == nil, false, "critical", "");
    snorlaxdbg(req == nil, false, "critical", "");
    snorlaxdbg(req->collection == nil, false, "critical", "");
#endif // RELEASE

    network_netlink_req_t * prev = req->prev;
    network_netlink_req_t * next = req->next;

    if(prev) {
        prev->next = next;
        req->prev = nil;
    } else {
        collection->head = next;
    }

    if(next) {
        next->prev = prev;
        req->next = nil;
    } else {
        collection->tail = prev;
    }

    collection->size = collection->size - 1;
    req->collection = nil;

    return req;
}