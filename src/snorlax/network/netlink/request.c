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

static network_netlink_request_func_t func = {
    (network_netlink_request_func_rem_t) buffer_list_node_func_rem,
    (network_netlink_request_func_front_t) buffer_list_node_func_front,
    (network_netlink_request_func_back_t) buffer_list_node_func_back,
    (network_netlink_request_func_length_t) buffer_list_node_func_length,
    (network_netlink_request_func_remain_t) buffer_list_node_func_remain,
    (network_netlink_request_func_position_get_t) buffer_list_node_func_position_get,
    (network_netlink_request_func_position_set_t) buffer_list_node_func_position_set,
    (network_netlink_request_func_size_get_t) buffer_list_node_func_size_get,
    (network_netlink_request_func_size_set_t) buffer_list_node_func_size_set,
    (network_netlink_request_func_capacity_get_t) buffer_list_node_func_capacity_get,
    (network_netlink_request_func_capacity_set_t) buffer_list_node_func_capacity_set,
    (network_netlink_request_func_clear_t) buffer_list_node_func_clear,
};

extern network_netlink_request_t * network_netlink_request_gen(buffer_list_t * buffer, struct nlmsghdr * nlmsg) {
    network_netlink_request_t * node = (network_netlink_request_t *) calloc(1, sizeof(network_netlink_request_t));

    node->func = address_of(func);

    buffer_list_add(buffer, (buffer_list_node_t *) node);

    uint64_t page = buffer->page ? buffer->page : 1;

    if(nlmsg) {
        node->capacity = nlmsg->nlmsg_len;
        node->size = nlmsg->nlmsg_len;
        node->message = nlmsg;
    }

    return node;
}