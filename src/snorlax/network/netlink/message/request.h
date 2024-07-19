/**
 * @file        snorlax/netlink/message/request.h
 * @brief
 * @details
 * 
 * @author      snorlax <ceo@snorlax.bio>
 * @since       July 18, 2024
 */

#ifndef   __NETWORK__NETLINK_MESSAGE_REQUEST__H__
#define   __NETWORK__NETLINK_MESSAGE_REQUEST__H__

#include <snorlax.h>
#include <snorlax/buffer/list.h>
#include <snorlax/network/netlink/message.h>

struct network_netlink_message_request;
struct network_netlink_message_request_func;

typedef struct network_netlink_message_request network_netlink_message_request_t;
typedef struct network_netlink_message_request_func network_netlink_message_request_func_t;

typedef void (*network_netlink_message_request_on_t)(network_netlink_message_request_t *, uint32_t);

struct network_netlink_message_request {
    network_netlink_message_request_func_t * func;
    sync_t * sync;
    buffer_list_t * collection;
    network_netlink_message_t * prev;
    network_netlink_message_t * next;

    uint32_t status;
    struct nlmsghdr * message;

    buffer_list_t * responses;

    network_netlink_message_request_on_t on;
};

struct network_netlink_message_request_func {
    network_netlink_message_request_t * (*rem)(network_netlink_message_request_t *);
    int32_t (*done)(network_netlink_message_request_t *);
};

extern network_netlink_message_request_t * network_netlink_message_request_gen(struct nlmsghdr * nlmsg, network_netlink_message_request_on_t on);

#endif // __NETWORK__NETLINK_MESSAGE_REQUEST__H__
