#include <sys/socket.h>
#include <linux/netlink.h>
#include <errno.h>
#include <time.h>
#include <sys/types.h>
#include <unistd.h>
#include <string.h>
#include <stdlib.h>

#include "netlink.h"

typedef network_netlink_t * (*network_netlink_rem_t)(___notnull network_netlink_t *);

typedef int32_t (*network_netlink_open_t)(___notnull network_netlink_t *);
typedef int64_t (*network_netlink_read_t)(___notnull network_netlink_t *);
typedef int64_t (*network_netlink_write_t)(___notnull network_netlink_t *);
typedef int32_t (*network_netlink_close_t)(___notnull network_netlink_t *);
typedef int32_t (*network_netlink_check_t)(___notnull network_netlink_t *, uint32_t);
typedef int32_t (*network_netlink_req_t)(___notnull network_netlink_t *, void *, network_netlink_res_t);

static network_netlink_t * network_netlink_func_rem(___notnull network_netlink_t * descriptor);
static int32_t network_netlink_func_open(___notnull network_netlink_t * descriptor);
static int32_t network_netlink_func_req(___notnull network_netlink_t * descriptor, struct nlmsghdr * req, network_netlink_res_t callback);

static network_netlink_func_t func = {
    (network_netlink_rem_t) descriptor_func_rem,
    (network_netlink_open_t) network_netlink_func_open,
    (network_netlink_read_t) descriptor_func_read,
    (network_netlink_write_t) descriptor_func_write,
    (network_netlink_close_t) descriptor_func_close,
    (network_netlink_check_t) descriptor_func_check,
    (network_netlink_req_t) network_netlink_func_req,
};

static const int buffer_size_in = 1024 * 1024;
static const int buffer_size_out = 32768;

static network_netlink_t * singleton = nil;

extern network_netlink_t * network_netlink_get(void) {
    if(singleton == nil) {
        singleton = network_netlink_gen(NETLINK_GENERIC);
        if(network_netlink_open(singleton) == fail) {
            return network_netlink_rem(singleton);
        }
    }
    return singleton;
}

extern network_netlink_t * network_netlink_gen(uint32_t subscriptions) {
    network_netlink_t * descriptor = (network_netlink_t *) calloc(1, sizeof(network_netlink_t));

    descriptor->func = address_of(func);

    descriptor->value = invalid;

    descriptor->buffer.in = buffer_gen(0);
    descriptor->buffer.out = buffer_gen(0);

    descriptor->status = descriptor_state_close;

    descriptor->subscriptions = subscriptions;
    descriptor->seq = time(nil);

    return descriptor;
}

static int32_t network_netlink_func_open(___notnull network_netlink_t * descriptor) {
#ifndef   RELEASE
    snorlaxdbg(descriptor == nil, false, "critical", "");
#endif // RELEASE

    if(descriptor->value <= invalid) {
        descriptor->value = socket(AF_NETLINK, SOCK_RAW | SOCK_CLOEXEC, NETLINK_ROUTE);

        if(descriptor->value <= invalid) {
            descriptor_exception_set(descriptor, descriptor_exception_type_system, errno, socket);

#ifndef   RELEASE
            snorlaxdbg(false, true, "warning", "fail to socket => %d", errno);
#endif // RELEASE

            return fail;
        }

        if(setsockopt(descriptor->value, SOL_SOCKET, SO_SNDBUF, &buffer_size_out, sizeof(buffer_size_out)) < 0) {
            descriptor_exception_set(descriptor, descriptor_exception_type_system, errno, setsockopt);

#ifndef   RELEASE
            snorlaxdbg(false, true, "warning", "fail to setsockopt => %d", errno);
#endif // RELEASE

            close(descriptor->value);
            descriptor->value = invalid;

            return fail;
        }

        if(setsockopt(descriptor->value, SOL_SOCKET, SO_RCVBUF, &buffer_size_out, sizeof(buffer_size_out)) < 0) {
            descriptor_exception_set(descriptor, descriptor_exception_type_system, errno, setsockopt);

#ifndef   RELEASE
            snorlaxdbg(false, true, "warning", "fail to setsockopt => %d", errno);
#endif // RELEASE

            close(descriptor->value);
            descriptor->value = invalid;

            return fail;
        }

        int one = 1;
        setsockopt(descriptor->value, SOL_NETLINK, NETLINK_EXT_ACK, &one, sizeof(one));

        struct sockaddr_nl local;
        memset(&local, 0, sizeof(struct sockaddr_nl));
        local.nl_family = AF_NETLINK;
        local.nl_groups = descriptor->subscriptions;

        if(bind(descriptor->value, (struct sockaddr *) &local, sizeof(struct sockaddr_nl)) < 0) {
            descriptor_exception_set(descriptor, descriptor_exception_type_system, errno, bind);

#ifndef   RELEASE
            snorlaxdbg(false, true, "warning", "fail to bind => %d", errno);
#endif // RELEASE

            close(descriptor->value);
            descriptor->value = invalid;

            return fail;
        }

        return success;
    }
    return success;
}

static int32_t network_netlink_func_req(___notnull network_netlink_t * descriptor, struct nlmsghdr * req, network_netlink_res_t callback) {
#ifndef   RELEASE
    snorlaxdbg(descriptor == nil, false, "critical", "");
    snorlaxdbg(descriptor->value <= invalid, false, "critical", "");
#endif // RELEASE

    struct iovec iov = {
        .iov_base = req,
        .iov_len = req->nlmsg_len
    };

    struct sockaddr_nl addr = { .nl_family = AF_NETLINK };
    struct msghdr msg = {
        .msg_name = &addr,
        .msg_namelen = sizeof(struct sockaddr_nl),
        .msg_iov = &iov,
        .msg_iovlen = 1
    };

    req->nlmsg_seq = ++(descriptor->seq);
    printf("request seq => %d\n", req->nlmsg_seq);
    printf("request len => %d\n", req->nlmsg_len);

    int32_t status = sendmsg(descriptor->value, &msg, 0);

    printf("status => %d\n", status);

    if(status < 0) {
#ifndef   RELEASE
        snorlaxdbg(false, true, "warning", "fail to sendmsg => %d", errno);
#endif // RELEASE
        return fail;
    }

    // BLOCKING CALL

    struct iovec res = { 0, };

    msg.msg_iov = &res;
    msg.msg_iovlen = 1;
    msg.msg_iov->iov_base = nil;
    msg.msg_iov->iov_len = 0;

    int32_t len = recvmsg(descriptor->value, &msg, MSG_PEEK | MSG_TRUNC);
    if(len < 0) {
#ifndef   RELEASE
        snorlaxdbg(false, true, "warning", "fail to recvmsg => %d", errno);
#endif // RELEASE
        return fail;
    }

    char * buffer = malloc(len);

    msg.msg_iov->iov_base = buffer;
    msg.msg_iov->iov_len = len;

    len = recvmsg(descriptor->value, &msg, 0);

    struct nlmsghdr * response = (struct nlmsghdr *) msg.msg_iov->iov_base;

    printf("response => %p\n", response);
    printf("response len => %d\n", response->nlmsg_len);
    printf("response type => %04x\n", response->nlmsg_type);
    printf("response seq => %d\n", response->nlmsg_seq);
    // printf("response message len => %d", res->nlmsg_len);

    if(response->nlmsg_type == NLMSG_ERROR) {
        struct nlmsgerr * err = (struct nlmsgerr *) NLMSG_DATA(response);
        printf("error => %d\n", err->error);
        // printf("", err->msg.)
    }

    return fail;
}