#include "response.h"
#include "../../netlink.h"

extern void network_netlink_message_response_debug(network_netlink_message_request_t * request, uint32_t state) {
#ifndef   RELEASE
    snorlaxdbg(request == nil, false, "critical", "");
#endif // RELEASE

    netlink_protocol_debug(stdout, request->message);

    for(network_netlink_message_t * node = (network_netlink_message_t *) request->responses->head; node != nil; node = node->next) {
        netlink_protocol_debug(stdout, node->message);
    }
}