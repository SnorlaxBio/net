/**
 * @file        snorlax/network/netlink.h
 * @brief
 * @details
 * 
 * @author      snorlax <ceo@snorlax.bio>
 * @since       July 7, 2024
 */

#ifndef   __SNORLAX__NETWORK_NETLINK__H__
#define   __SNORLAX__NETWORK_NETLINK__H__

#include <snorlax.h>
#include <snorlax/descriptor.h>
#include <snorlax/network/netlink/req/list.h>

struct network_netlink;
struct network_netlink_func;
struct network_netlink_req;

typedef struct network_netlink network_netlink_t;
typedef struct network_netlink_func network_netlink_func_t;
typedef struct network_netlink_req network_netlink_req_t;

struct network_netlink {
    network_netlink_func_t * func;
    sync_t * sync;
    descriptor_buffer_t buffer;
    descriptor_exception_t exception;
    int32_t value;
    uint32_t status;
    uint32_t subscriptions;
    uint32_t seq;
    uint64_t transfer;          // OVERFLOW 가 발생했을 때, 체크를 해야 한다.... 
    network_netlink_req_list_t * requests;
};

// typedef void (*network_netlink_res_t)(void *);

struct network_netlink_func {
    network_netlink_t * (*rem)(___notnull network_netlink_t *);

    int32_t (*open)(___notnull network_netlink_t *);
    int64_t (*read)(___notnull network_netlink_t *);
    int64_t (*write)(___notnull network_netlink_t *);
    int32_t (*close)(___notnull network_netlink_t *);
    int32_t (*check)(___notnull network_netlink_t *, uint32_t);

    network_netlink_req_t * (*req)(___notnull network_netlink_t *, void *, network_netlink_res_callback_t);
};

extern network_netlink_t * network_netlink_gen(uint32_t subscriptions);

extern network_netlink_t * network_netlink_get(void);

#define network_netlink_rem(descriptor)                             ((descriptor)->func->rem(descriptor))
#define network_netlink_open(descriptor)                            ((descriptor)->func->open(descriptor))
#define network_netlink_read(descriptor)                            ((descriptor)->func->read(descriptor))
#define network_netlink_write(descriptor)                           ((descriptor)->func->write(descriptor))
#define network_netlink_close(descriptor)                           ((descriptor)->func->close(descriptor))
#define network_netlink_check(descriptor, state)                    ((descriptor)->func->check(descriptor, state))
#define network_netlink_request_pop(descriptor, request, clear)     ((request *) buffer_pop((descriptor)->buffer.out, sizeof(request), clear))
#define network_netlink_request_rel(descriptor, request)            ((request *) buffer_rel((descriptor)->buffer.out, sizeof(request)))
#define network_netlink_req(descriptor, request, callback)          ((descriptor)->func->req(descriptor, request, callback))

#include <linux/netlink.h>

#define NLMSG_TAIL(req)    ((struct rtattr *) (((void *) (&(req)->header)) + NLMSG_ALIGN((req)->header.nlmsg_len)))

#define netlink_protocol_data_get(type, address, len)               ((type)(((void *) address) + NLMSG_ALIGN(len)))
#define netlink_protocol_data_end(header)                           ((((void *)(header)) + NLMSG_ALIGN(header->nlmsg_len)))
#define netlink_protocol_attr_next(attr)                            ((struct rtattr *)(((void *)(attr))  + RTA_ALIGN((attr)->rta_len)))

// netlink_protocol_data_end(header); attr = netlink_protocol_attr_next

// 0x560c6d66db08 < 0x560c6d66db58
// attr->rta_type => 1
// attr->rta_len => 8
// 0x560c6d66db08
// attr->rta_type => 2
// attr->rta_len => 8
// 0x560c6d66db10
// attr->rta_type => 4
// attr->rta_len => 8
// 0x560c6d66db18
// attr->rta_type => 3
// attr->rta_len => 9
// 0x560c6d66db20
// attr->rta_type => 2048
// attr->rta_len => 0
// 0x560c6d66db29

extern void netlink_protocol_debug(FILE * stream, void * data);

#endif // __SNORLAX__NETWORK_NETLINK__H__
