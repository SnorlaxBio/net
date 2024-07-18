#include "request.h"

static network_netlink_message_request_t * network_netlink_message_request_func_rem(network_netlink_message_request_t * request);

static network_netlink_message_request_func_t func = {
    network_netlink_message_request_func_rem
};

extern network_netlink_message_request_t * network_netlink_message_request_gen(struct nlmsghdr * nlmsg, network_netlink_message_request_on_t on) {
    network_netlink_message_request_t * request = (network_netlink_message_request_t *) calloc(1, sizeof(network_netlink_message_request_t));

    request->func = address_of(func);

    if(nlmsg) request->message = memory_dup(nlmsg, nlmsg->nlmsg_len);

    request->responses = buffer_list_gen();
    request->on = on;

    return request;
}

static network_netlink_message_request_t * network_netlink_message_request_func_rem(network_netlink_message_request_t * request) {
#ifndef   RELEASE
    snorlaxdbg(request == nil, false, "critical", "");
#endif // RELEASE

    if(request->collection) buffer_list_del(request->collection, (buffer_list_node_t *) request);

    request->message = memory_rem(request->message);
    request->responses = buffer_list_rem(request->responses);
    request->sync = sync_rem(request->sync);

    free(request);

    return nil;
}
