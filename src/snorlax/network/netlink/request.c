#include "request.h"

typedef network_netlink_request_t * (*network_netlink_request_func_rem_t)(network_netlink_request_t *);
typedef void * (*network_netlink_request_func_front_t)(network_netlink_request_t *);
typedef void * (*network_netlink_request_func_back_t)(network_netlink_request_t *);
typedef uint64_t (*network_netlink_request_func_length_t)(network_netlink_request_t *);
typedef uint64_t (*network_netlink_request_func_remain_t)(network_netlink_request_t *);
typedef uint64_t (*network_netlink_request_func_position_get_t)(network_netlink_request_t *);
typedef void (*network_netlink_request_func_position_set_t)(network_netlink_request_t *, uint64_t);
typedef uint64_t (*network_netlink_request_func_size_get_t)(network_netlink_request_t *);
typedef void (*network_netlink_request_func_size_set_t)(network_netlink_request_t *, uint64_t);
typedef uint64_t (*network_netlink_request_func_capacity_get_t)(network_netlink_request_t *);
typedef void (*network_netlink_request_func_capacity_set_t)(network_netlink_request_t *, uint64_t);
typedef void (*network_netlink_request_func_clear_t)(network_netlink_request_t *);

static int32_t network_netlink_request_func_shrink(network_netlink_request_t * node);
static struct nlmsghdr * network_netlink_request_func_nlmsghdr_get(network_netlink_request_t * node);
static uint64_t network_netlink_request_func_done_get(network_netlink_request_t * node);
static void network_netlink_request_func_done_set(network_netlink_request_t * node, uint64_t v);

static network_netlink_request_func_t func = {
    (network_netlink_request_func_rem_t) buffer_list_node_func_rem,
    (network_netlink_request_func_front_t) buffer_list_node_func_front,
    (network_netlink_request_func_back_t) buffer_list_node_func_back,
    network_netlink_request_func_shrink,
    (network_netlink_request_func_length_t) buffer_list_node_func_length,
    (network_netlink_request_func_remain_t) buffer_list_node_func_remain,
    (network_netlink_request_func_position_get_t) buffer_list_node_func_position_get,
    (network_netlink_request_func_position_set_t) buffer_list_node_func_position_set,
    (network_netlink_request_func_size_get_t) buffer_list_node_func_size_get,
    (network_netlink_request_func_size_set_t) buffer_list_node_func_size_set,
    (network_netlink_request_func_capacity_get_t) buffer_list_node_func_capacity_get,
    (network_netlink_request_func_capacity_set_t) buffer_list_node_func_capacity_set,
    (network_netlink_request_func_clear_t) buffer_list_node_func_clear,
    network_netlink_request_func_nlmsghdr_get,
    network_netlink_request_func_done_get,
    network_netlink_request_func_done_set
};

extern network_netlink_request_t * network_netlink_request_gen(buffer_list_t * buffer, struct nlmsghdr * nlmsg, uint64_t n) {
    network_netlink_request_t * node = (network_netlink_request_t *) calloc(1, sizeof(network_netlink_request_t));

    node->func = address_of(func);

    buffer_list_add(buffer, (buffer_list_node_t *) node);

    uint64_t page = buffer->page ? buffer->page : 1;

    if(nlmsg) {
        node->capacity = nlmsg->nlmsg_len;
        node->size = nlmsg->nlmsg_len;
        node->message = nlmsg;
    } else if(n > 0) {
        node->capacity = n;
        node->message = (struct nlmsghdr *) malloc(n);
    }

    return node;
}

static int32_t network_netlink_request_func_shrink(network_netlink_request_t * node) {
#ifndef   RELEASE
    snorlaxdbg(node == nil, false, "critical", "");
#endif // RELEASE

    if(node->done == node->size) {
        buffer_list_del(node->collection, (buffer_list_node_t *) node);
        network_netlink_request_rem(node);

        return success;
    }

    return fail;
}

static struct nlmsghdr * network_netlink_request_func_nlmsghdr_get(network_netlink_request_t * node) {
#ifndef   RELEASE
    snorlaxdbg(node == nil, false, "critical", "");
#endif // RELEASE

    uint8_t * mem = (uint8_t *) node->message;

    return node->size <= node->done ? nil : (struct nlmsghdr *) &mem[node->done];
}

static uint64_t network_netlink_request_func_done_get(network_netlink_request_t * node) {
#ifndef   RELEASE
    snorlaxdbg(node == nil, false, "critical", "");
#endif // RELEASE

    return node->done;
}

static void network_netlink_request_func_done_set(network_netlink_request_t * node, uint64_t v) {
#ifndef   RELEASE
    snorlaxdbg(node == nil, false, "critical", "");
    snorlaxdbg(node->size < v, false, "critical", "");
#endif // RELEASE

    node->done = v;
}
