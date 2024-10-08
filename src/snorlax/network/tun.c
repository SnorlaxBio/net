#include <sys/ioctl.h>
#include <fcntl.h>
#include <errno.h>
#include <net/if.h>
#include <linux/if_tun.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <string.h>
#include <unistd.h>
#include <stdlib.h>
#include <arpa/inet.h>

#include <snorlax/socket/client.h>
#include <snorlax/buffer/mem.h>

#include "tun.h"

#include "netlink.h"
#include "netlink/message.h"

typedef network_tun_t * (*network_tun_func_rem_t)(___notnull network_tun_t *);
typedef int32_t (*network_tun_func_open_t)(___notnull network_tun_t *);
typedef int64_t (*network_tun_func_read_t)(___notnull network_tun_t *);
typedef int64_t (*network_tun_func_write_t)(___notnull network_tun_t *);
typedef int32_t (*network_tun_func_close_t)(___notnull network_tun_t *);
typedef int32_t (*network_tun_func_check_t)(___notnull network_tun_t *, uint32_t);

static int32_t network_tun_func_protect(___notnull network_tun_t * descriptor, ___notnull descriptor_t * s);

static network_tun_func_t func = {
    (network_tun_func_rem_t) descriptor_func_rem,
    (network_tun_func_open_t) network_tun_func_open,
    (network_tun_func_read_t) descriptor_func_read,
    (network_tun_func_write_t) descriptor_func_write,
    (network_tun_func_close_t) descriptor_func_close,
    (network_tun_func_check_t) descriptor_func_check,
    network_tun_func_protect
};

extern network_tun_t * network_tun_gen(void) {
    network_tun_t * descriptor = (network_tun_t *) calloc(1, sizeof(network_tun_t));

    descriptor->func = address_of(func);

    descriptor->value = invalid;

    descriptor->buffer.in = (buffer_t *) buffer_mem_gen(0);
    descriptor->buffer.out = (buffer_t *) buffer_mem_gen(0);

    descriptor->status = descriptor_state_close;

    return descriptor;
}

extern int32_t network_tun_func_open(network_tun_t * descriptor) {
#ifndef   RELEASE
    snorlaxdbg(descriptor == nil, false, "critical", "");
#endif // RELEASE

    if(descriptor->value <= invalid) {
        descriptor->value = open("/dev/net/tun", O_RDWR);

        if(descriptor->value <= invalid) {
#ifndef   RELEASE
            snorlaxdbg(descriptor->value <= invalid, false, "critical", "fail to open => %d", errno);
#endif // RELEASE
            descriptor_exception_set(descriptor, descriptor_exception_type_system, errno, open);

            return fail;
        }

        descriptor->status = descriptor->status | (descriptor_state_open | descriptor_state_write);
        descriptor->status = descriptor->status & (~descriptor_state_close);

        struct ifreq req;
        memset(&req, 0, sizeof(req));

        req.ifr_flags = req.ifr_flags | IFF_MULTI_QUEUE;    // IFF_ONE_QUEUE
        req.ifr_flags = req.ifr_flags | IFF_NO_PI;          // IFF_ONE_QUEUE
        req.ifr_flags = req.ifr_flags | IFF_TUN;

        if(ioctl(descriptor->value, TUNSETIFF, &req) < 0) {
#ifndef   RELEASE
            snorlaxdbg(true, false, "critical", "fail to ioctl => %d", errno);
#endif // RELEASE
            descriptor_exception_set(descriptor, descriptor_exception_type_system, errno, open);

            return fail;
        }

        // TX QUEUE SET: REFACTOR USE NETLINK
        int fd = socket(AF_INET, SOCK_DGRAM, 0);
        if(fd > 0) {
            req.ifr_qlen = ETH_DATA_LEN;
            if(ioctl(fd, SIOCSIFTXQLEN, &req) < 0) {
#ifndef   RELEASE
                snorlaxdbg(false, true, "warning", "fail to ioctl(...) caused by %d", errno);
#endif // RELEASE
            }

            close(fd);
        } else {
#ifndef   RELEASE
            snorlaxdbg(false, true, "warning", "fail to socket(...) => %d", errno);
#endif // RELEASE
        }

        descriptor_nonblock_on((descriptor_t *) descriptor);

#if 0
        uint8_t all[4] = { 0, 0, 0, 0 };
        uint8_t addr[4] = { 10, 0, 0, 1 };

        // DEFAULT ROUTE GET

        // REMOVE MEMORY ALLOCATION ...
        // network_netlink_wait(network_netlink_get(), network_netlink_req(network_netlink_get(), network_netlink_message_iproute_get_gen(), network_netlink_message_response_debug));
        network_netlink_wait(network_netlink_get(), network_netlink_req(network_netlink_get(), network_netlink_message_ipaddr_add_gen(AF_INET, addr, 24, "tun0"), nil));
        network_netlink_wait(network_netlink_get(), network_netlink_req(network_netlink_get(), network_netlink_message_iplink_setup_gen("tun0"), nil));
        network_netlink_wait(network_netlink_get(), network_netlink_req(network_netlink_get(), network_netlink_message_iprule_add_gen(network_netlink_table_main_mark, network_netlink_table_main_priority, network_netlink_table_main_id), nil));
        network_netlink_wait(network_netlink_get(), network_netlink_req(network_netlink_get(), network_netlink_message_iprule_add_gen(network_netlink_table_tun_mark, network_netlink_table_tun_priority, network_netlink_table_tun_id), nil));
        network_netlink_wait(network_netlink_get(), network_netlink_req(network_netlink_get(), network_netlink_message_iproute_prepend_gen(all, 0, addr, network_netlink_table_tun_id), nil));
#endif // 0

    } else {
#ifndef   RELEASE
        snorlaxdbg(false, true, "warning", "");
#endif // RELEASE
    }

    return success;
}

static int32_t network_tun_func_protect(___notnull network_tun_t * descriptor, ___notnull descriptor_t * s) {
#ifndef   RELEASE
//    snorlaxdbg(descriptor == nil, false, "critical", "");
    snorlaxdbg(s == nil, false, "critical", "");
#endif // RELEASE

    if(s->value > invalid) {
        int mark = network_netlink_table_main_mark;
        if(setsockopt(s->value, SOL_SOCKET, SO_MARK, &mark, sizeof(mark)) == 0) {
            return success;
        }
#ifndef   RELEASE
        snorlaxdbg(false, true, "warning", "fail to setsockopt => %d", errno);
#endif // RELEASE
    }

    return fail;
}