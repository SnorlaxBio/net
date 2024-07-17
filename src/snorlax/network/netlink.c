#include <sys/socket.h>
#include <linux/netlink.h>
#include <linux/rtnetlink.h>
#include <errno.h>
#include <time.h>
#include <sys/types.h>
#include <unistd.h>
#include <string.h>
#include <stdlib.h>
#include <poll.h>

#include <snorlax/buffer/list.h>

#include "netlink.h"
#include "netlink/message.h"
#include "netlink/message/state.h"

static const int socket_buffer_size_out = 32768;
static const int socket_buffer_size_in = 1024 * 1024;
static const int enable = 1;
static network_netlink_t * singleton = nil;

static network_netlink_t * network_netlink_func_rem(___notnull network_netlink_t * descriptor);
static int32_t network_netlink_func_open(___notnull network_netlink_t * descriptor);
static int64_t network_netlink_func_read(___notnull network_netlink_t * descriptor);
static int64_t network_netlink_func_write(___notnull network_netlink_t * descriptor);
static int32_t network_netlink_func_close(___notnull network_netlink_t * descriptor);
static int32_t network_netlink_func_check(___notnull network_netlink_t * descriptor, uint32_t state);
static network_netlink_message_t * network_netlink_func_req(___notnull network_netlink_t * descriptor, struct nlmsghdr * message);
static int32_t network_netlink_func_wait(___notnull network_netlink_t * descriptor, ___notnull network_netlink_message_t * request);

static network_netlink_func_t func = {
    network_netlink_func_rem,
    network_netlink_func_open,
    network_netlink_func_read,
    network_netlink_func_write,
    network_netlink_func_close,
    network_netlink_func_check,
    network_netlink_func_req,
    network_netlink_func_wait
};

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
    descriptor->buffer.in = buffer_list_gen();
    descriptor->buffer.out = buffer_list_gen();
    descriptor->value = invalid;
    descriptor->status = descriptor->status | descriptor_state_close;
    descriptor->subscriptions = subscriptions;
    descriptor->seq = time(nil);

    return descriptor;
}

static network_netlink_t * network_netlink_func_rem(___notnull network_netlink_t * descriptor) {
#ifndef   RELEASE
    snorlaxdbg(descriptor == nil, false, "critical", "");
    snorlaxdbg(descriptor->value > invalid, false, "critical", "");
#endif // RELEASE

    descriptor->buffer.in = buffer_list_rem(descriptor->buffer.in);
    descriptor->buffer.out = buffer_list_rem(descriptor->buffer.out);
    descriptor->sync = sync_rem(descriptor->sync);

    free(descriptor);

    return nil;
}

static int32_t network_netlink_func_open(___notnull network_netlink_t * descriptor) {
#ifndef   RELEASE
    snorlaxdbg(descriptor == nil, false, "critical", "");
#endif // RELEASE

    if(descriptor->value <= invalid) {
        descriptor->value = socket(AF_NETLINK, SOCK_RAW | SOCK_CLOEXEC, NETLINK_ROUTE);

        if(descriptor->value <= invalid) {
            descriptor_exception_set(descriptor, descriptor_exception_type_system, errno, socket);
            return fail;
        }

        if(setsockopt(descriptor->value, SOL_SOCKET, SO_SNDBUF, &socket_buffer_size_out, sizeof(socket_buffer_size_out)) < 0) {
            descriptor_exception_set(descriptor, descriptor_exception_type_system, errno, setsockopt);
            return fail;
        }

        if(setsockopt(descriptor->value, SOL_SOCKET, SO_RCVBUF, &socket_buffer_size_in, sizeof(socket_buffer_size_in)) < 0) {
            descriptor_exception_set(descriptor, descriptor_exception_type_system, errno, setsockopt);
            return fail;
        }

        if(setsockopt(descriptor->value, SOL_NETLINK, NETLINK_EXT_ACK, &enable, sizeof(enable)) < 0) {
            descriptor_exception_set(descriptor, descriptor_exception_type_system, errno, setsockopt);
            return fail;
        }

        struct sockaddr_nl addr;
        memset(&addr, 0, sizeof(struct sockaddr_nl));
        addr.nl_family = AF_NETLINK;
        addr.nl_groups = descriptor->subscriptions;

        if(bind(descriptor->value, (struct sockaddr *) &addr, sizeof(struct sockaddr_nl)) < 0) {
            descriptor_exception_set(descriptor, descriptor_exception_type_system, errno, bind);
            return fail;
        }

        descriptor_nonblock_on((descriptor_t *) descriptor);

        descriptor->status = descriptor->status | (descriptor_state_open | descriptor_state_write);
    } else {
#ifndef   RELEASE
        snorlaxdbg(false, true, "caution", "");
#endif // RELEASE
    }

    return success;
}

static int64_t network_netlink_func_read(___notnull network_netlink_t * descriptor) {
#ifndef   RELEASE
    snorlaxdbg(descriptor == nil, false, "critical", "");
#endif // RELEASE

    if(descriptor->value > invalid) {
        if(descriptor->status & descriptor_state_open_in) {
            buffer_list_t * in = descriptor->buffer.in;
            buffer_list_t * out = descriptor->buffer.out;

            /**
             * Need to upgrade : 현재의 버퍼는 로컬 스택에 저장하고 그것을 메시지에 담는다.
             * 이것은 비효율적이다. memcpy 와 read 를 동시에 수행할 수 있는 버퍼 구조를 가져야 한다.
             */

            // 8192 to avoid message truncation on platforms with page size > 4096
            struct nlmsghdr buffer[8192/sizeof(struct nlmsghdr)];
            struct iovec iov = { buffer, sizeof(buffer) };
            struct sockaddr_nl addr;
            struct msghdr msg = { &addr, sizeof(addr), &iov, 1, NULL, 0, 0 };

            int64_t n = recvmsg(descriptor->value, &msg, 0);

            if(n > 0) {
                descriptor->status = descriptor->status | descriptor_state_read;

                for(struct nlmsghdr * message = (struct nlmsghdr *) buffer; NLMSG_OK(message, n); message = NLMSG_NEXT(message, n)) {
                    network_netlink_message_t * node = network_netlink_message_gen(message);
                    buffer_list_push(in, (buffer_list_node_t *) node);
                    in->back = (buffer_list_node_t *) node;
                    if(message->nlmsg_type == NLMSG_ERROR){
                        node->status = node->status | network_netlink_message_state_res;
                    }
                }
            } else if(n == 0) {
                descriptor->status = descriptor->status & (~descriptor_state_read);
#ifndef   RELEASE
                snorlaxdbg(false, true, "warning", "");
#endif // RELEASE
                descriptor_exception_set(descriptor, descriptor_exception_type_lib, descriptor_exception_no_eof, recvmsg);

                n = fail;
            } else {
                descriptor->status = descriptor->status & (~descriptor_state_read);

                if(errno == EAGAIN) {
                    n = 0;
                } else {
#ifndef   RELEASE
                    snorlaxdbg(false, true, "descriptor exception", "%d %d %p", descriptor_exception_type_system, errno, recvmsg);
#endif // RELEASE
                    descriptor_exception_set(descriptor, descriptor_exception_type_system, errno, recvmsg);
                }
            }

            return n;
        } else {
#ifndef   RELEASE
            snorlaxdbg(false, true, "warning", "");
#endif // RELEASE
        }
    } else {
#ifndef   RELEASE
        snorlaxdbg(false, true, "warning", "");
#endif // RELEASE
    }

    return fail;
}

static int64_t network_netlink_func_write(___notnull network_netlink_t * descriptor) {
#ifndef   RELEASE
    snorlaxdbg(descriptor == nil, false, "critical", "");
#endif // RELEASE

    if(descriptor->value > invalid) {
        if(descriptor->status & descriptor_state_open_out) {
            buffer_list_t * out = descriptor->buffer.out;
            struct sockaddr_nl addr;
            memset(&addr, 0, sizeof(addr));
            addr.nl_family = AF_NETLINK;


            for(network_netlink_message_t * node = (network_netlink_message_t *) out->front; node != nil; node = node->next) {
                netlink_protocol_debug(stdout, node->message);
                struct iovec iov = { node->message, node->message->nlmsg_len };
                struct msghdr msg = { &addr, sizeof(addr), &iov, 1, NULL, 0, 0 };
                node->message->nlmsg_flags = node->message->nlmsg_flags | NLM_F_ACK;
                node->message->nlmsg_pid = 0;
                node->message->nlmsg_seq = (++descriptor->seq);

                int64_t n = sendmsg(descriptor->value, &msg, 0);

                if(n > 0) {
#ifndef   RELEASE
                    snorlaxdbg(n != node->message->nlmsg_len, false, "critical", "");
#endif // RELEASE
                    node->status = node->status | network_netlink_message_state_req;
                    descriptor->status = descriptor->status | descriptor_state_write;
                    out->front = (buffer_list_node_t *) node->next;
                    continue;
                } else if(n == 0) {
                    descriptor->status = descriptor->status & (~descriptor_state_write);

#ifndef   RELEASE
                    snorlaxdbg(n == 0, true, "critical", "");
#endif // RELEASE
                    return fail;
                } else {
                    descriptor->status = descriptor->status & (~descriptor_state_write);

                    if(errno == EAGAIN) {
                        return success;
                    } else {
                        descriptor_exception_set(descriptor, descriptor_exception_type_system, errno, sendmsg);
                    }
                }
            }
            return success;
        } else {
#ifndef   RELEASE
            snorlaxdbg(false, true, "warning", "");
#endif // RELEASE
        }
    } else {
#ifndef   RELEASE
        snorlaxdbg(false, true, "warning", "");
#endif // RELEASE
    }
}

static int32_t network_netlink_func_close(___notnull network_netlink_t * descriptor) {
#ifndef   RELEASE
    snorlaxdbg(descriptor == nil, false, "critical", "");
#endif // RELEASE

    if(descriptor->value > invalid) {
        if(close(descriptor->value) == fail) {
#ifndef   RELEASE
            snorlaxdbg(false, true, "notice", "fail to close => %d", errno);
#endif // RELEASE
        }
        descriptor->value = invalid;
    }

    descriptor->status = descriptor->status & (~(descriptor_state_open | descriptor_state_read | descriptor_state_write));
    descriptor->status = descriptor->status | descriptor_state_close;

    descriptor_exception_set(descriptor, descriptor_exception_type_none, descriptor_exception_no_none, nil);

    return success;
}

static int32_t network_netlink_func_check(___notnull network_netlink_t * descriptor, uint32_t state) {
#ifndef   RELEASE
    snorlaxdbg(descriptor == nil, false, "critical", "");
#endif // RELEASE

    return true;
}

static network_netlink_message_t * network_netlink_func_req(___notnull network_netlink_t * descriptor, struct nlmsghdr * message) {
#ifndef   RELEASE
    snorlaxdbg(descriptor == nil, false, "critical", "");
    snorlaxdbg(message == nil, false, "critical", ""); 
#endif // RELEASE

    network_netlink_message_t * node = network_netlink_message_gen(nil);

    node->message = message;

    buffer_list_push(descriptor->buffer.out, (buffer_list_node_t *) node);

    if(descriptor->buffer.out->front == nil) descriptor->buffer.out->front = (buffer_list_node_t *) node;

    network_netlink_write(descriptor);

    // TODO: ERROR HANDLING

    return node;
}

static int32_t network_netlink_func_wait(___notnull network_netlink_t * descriptor, ___notnull network_netlink_message_t * request) {
#ifndef   RELEASE
    snorlaxdbg(descriptor == nil, false, "critical", "");
    snorlaxdbg(request == nil, false, "critical", "");
#endif // RELEASE

    if(descriptor->value > invalid) {
        struct pollfd fds[1];
        buffer_list_t * in = descriptor->buffer.in;
        buffer_list_t * out = descriptor->buffer.out;
        network_netlink_write(descriptor);

        // ERROR HANDLING

        while(descriptor_exception_get(descriptor) == nil) {
            fds[0].fd = descriptor->value;
            fds[0].events = (POLLIN | POLLPRI | POLLERR | POLLHUP | POLLNVAL);
            if(out->front) fds[0].events = fds[0].events | POLLOUT;
            fds[0].revents = 0;

            int n = poll(fds, 1, 1);

            for(int i = 0; i < n; i++) {
                if(fds[i].revents & (POLLPRI | POLLERR | POLLHUP | POLLNVAL)) {
                    int error = 0;
                    socklen_t errorlen = sizeof(error);
                    if(getsockopt(descriptor->value, SOL_SOCKET, SO_ERROR, &error, &errorlen) >= 0) {
                        descriptor_exception_set(descriptor, descriptor_exception_type_system, error, poll);
                    } else {
                        descriptor_exception_set(descriptor, descriptor_exception_type_system, fds[i].revents, poll);
                    }
                    break;
                }
                if(fds[i].revents & POLLOUT) {
                    network_netlink_write(descriptor);
                }
                if(fds[i].revents & POLLIN) {
                    network_netlink_read(descriptor);
                    for(network_netlink_message_t * node = (network_netlink_message_t *) in->tail; node != nil; node = node->prev) {
                        if(request->message->nlmsg_seq == node->message->nlmsg_seq) {
                            netlink_protocol_debug(stdout, request->message);
                            if(node->status & network_netlink_message_state_res) {
#ifndef   RELEASE
                                snorlaxdbg(false, true, "response", "");
#endif // RELEASE
                                return success;
                            }
                        }
                    }
                }
            }
        }


    } else {
#ifndef   RELEASE
        snorlaxdbg(descriptor->value <= invalid, false, "critical", "");
#endif // RELEASE
    }

    return fail;
}

// typedef network_netlink_t * (*network_netlink_rem_t)(___notnull network_netlink_t *);

// typedef int32_t (*network_netlink_open_t)(___notnull network_netlink_t *);
// typedef int64_t (*network_netlink_read_t)(___notnull network_netlink_t *);
// typedef int64_t (*network_netlink_write_t)(___notnull network_netlink_t *);
// typedef int32_t (*network_netlink_close_t)(___notnull network_netlink_t *);
// typedef int32_t (*network_netlink_check_t)(___notnull network_netlink_t *, uint32_t);
// // typedef int32_t (*network_netlink_req_t)(___notnull network_netlink_t *, void *, network_netlink_res_t);

// static network_netlink_t * network_netlink_func_rem(___notnull network_netlink_t * descriptor);
// static int32_t network_netlink_func_open(___notnull network_netlink_t * descriptor);
// static int64_t network_netlink_func_read(___notnull network_netlink_t * descriptor);
// static int64_t network_netlink_func_write(___notnull network_netlink_t * descriptor);
// static network_netlink_req_t * network_netlink_func_req(___notnull network_netlink_t * descriptor, void * req, network_netlink_res_callback_t callback);
// static void network_netlink_func_flush(___notnull network_netlink_t * descriptor, network_netlink_is_flushed_t check);
// static int32_t network_netlink_func_wait(___notnull network_netlink_t * descriptor, network_netlink_req_t * req);

// static network_netlink_func_t func = {
//     (network_netlink_rem_t) network_netlink_func_rem,
//     (network_netlink_open_t) network_netlink_func_open,
//     network_netlink_func_read,
//     network_netlink_func_write,
//     (network_netlink_close_t) descriptor_func_close,
//     (network_netlink_check_t) descriptor_func_check,
//     network_netlink_func_req,
//     network_netlink_func_flush,
//     network_netlink_func_wait
// };

// static const int buffer_size_in = 1024 * 1024;
// static const int buffer_size_out = 32768;

// static network_netlink_t * singleton = nil;

// extern network_netlink_t * network_netlink_get(void) {
//     if(singleton == nil) {
//         singleton = network_netlink_gen(NETLINK_GENERIC);
//         if(network_netlink_open(singleton) == fail) {
//             return network_netlink_rem(singleton);
//         }
//     }
//     return singleton;
// }

// extern network_netlink_t * network_netlink_gen(uint32_t subscriptions) {
//     network_netlink_t * descriptor = (network_netlink_t *) calloc(1, sizeof(network_netlink_t));

//     descriptor->func = address_of(func);

//     descriptor->value = invalid;

//     descriptor->buffer.in = buffer_list_gen();
//     descriptor->buffer.out = buffer_list_gen(0);

//     descriptor->status = descriptor_state_close;

//     descriptor->subscriptions = subscriptions;
//     descriptor->seq = time(nil);

//     descriptor->requests = network_netlink_req_list_gen();

//     return descriptor;
// }

// static network_netlink_t * network_netlink_func_rem(___notnull network_netlink_t * descriptor) {
// #ifndef   RELEASE
//     snorlaxdbg(descriptor == nil, false, "critical", "");
// #endif // RELEASE

//     descriptor->buffer.in = buffer_rem(descriptor->buffer.in);
//     descriptor->buffer.out = buffer_rem(descriptor->buffer.out);

//     descriptor->sync = sync_rem(descriptor->sync);

// #ifndef   RELEASE
//     snorlaxdbg(false, true, "implement", "");
// #endif // RELEASE

//     descriptor->requests = network_netlink_req_list_rem(descriptor->requests);

//     free(descriptor);

//     return nil;
// }

// static int32_t network_netlink_func_open(___notnull network_netlink_t * descriptor) {
// #ifndef   RELEASE
//     snorlaxdbg(descriptor == nil, false, "critical", "");
// #endif // RELEASE

//     if(descriptor->value <= invalid) {
//         descriptor->value = socket(AF_NETLINK, SOCK_RAW | SOCK_CLOEXEC, NETLINK_ROUTE);

//         if(descriptor->value <= invalid) {
//             descriptor_exception_set(descriptor, descriptor_exception_type_system, errno, socket);

// #ifndef   RELEASE
//             snorlaxdbg(false, true, "warning", "fail to socket => %d", errno);
// #endif // RELEASE

//             return fail;
//         }

//         if(setsockopt(descriptor->value, SOL_SOCKET, SO_SNDBUF, &buffer_size_out, sizeof(buffer_size_out)) < 0) {
//             descriptor_exception_set(descriptor, descriptor_exception_type_system, errno, setsockopt);

// #ifndef   RELEASE
//             snorlaxdbg(false, true, "warning", "fail to setsockopt => %d", errno);
// #endif // RELEASE

//             close(descriptor->value);
//             descriptor->value = invalid;

//             return fail;
//         }

//         if(setsockopt(descriptor->value, SOL_SOCKET, SO_RCVBUF, &buffer_size_out, sizeof(buffer_size_out)) < 0) {
//             descriptor_exception_set(descriptor, descriptor_exception_type_system, errno, setsockopt);

// #ifndef   RELEASE
//             snorlaxdbg(false, true, "warning", "fail to setsockopt => %d", errno);
// #endif // RELEASE

//             close(descriptor->value);
//             descriptor->value = invalid;

//             return fail;
//         }

//         int one = 1;
//         setsockopt(descriptor->value, SOL_NETLINK, NETLINK_EXT_ACK, &one, sizeof(one));

//         struct sockaddr_nl local;
//         memset(&local, 0, sizeof(struct sockaddr_nl));
//         local.nl_family = AF_NETLINK;
//         local.nl_groups = descriptor->subscriptions;

//         if(bind(descriptor->value, (struct sockaddr *) &local, sizeof(struct sockaddr_nl)) < 0) {
//             descriptor_exception_set(descriptor, descriptor_exception_type_system, errno, bind);

// #ifndef   RELEASE
//             snorlaxdbg(false, true, "warning", "fail to bind => %d", errno);
// #endif // RELEASE

//             close(descriptor->value);
//             descriptor->value = invalid;

//             return fail;
//         }

//         return success;
//     }

//     return success;
// }

// /**
//  * poll
//  * 
//  * 
//  * @todo        upgrade logic / poll / nonblocking ...
//  */
// static network_netlink_req_t * network_netlink_func_req(___notnull network_netlink_t * descriptor, void * o, network_netlink_res_callback_t callback) {
// #ifndef   RELEASE
//     snorlaxdbg(descriptor == nil, false, "critical", "");
//     snorlaxdbg(descriptor->value <= invalid, false, "critical", "");
// #endif // RELEASE

//     // struct nlmsghdr * nlmsg = (struct nlmsghdr *) o;

//     // descriptor->seq = descriptor->seq + 1;

//     // nlmsg->nlmsg_seq = descriptor->seq;

//     // network_netlink_req_t * req = network_netlink_req_gen(descriptor->requests, nlmsg->nlmsg_seq, nlmsg, nlmsg->nlmsg_len, callback);


//     // req->start = descriptor->transfer + buffer_length(descriptor->buffer.out);
//     // struct msghdr * msg = network_netlink_request_pop(descriptor, struct msghdr, true);
//     // req->end = descriptor->transfer + nlmsg->nlmsg_len;

//     // netlink_protocol_debug(stdout, nlmsg);

//     // struct iovec iov = {
//     //     .iov_base = nlmsg,
//     //     .iov_len = nlmsg->nlmsg_len
//     // };

//     // struct sockaddr_nl addr = { .nl_family = AF_NETLINK };

//     // msg->msg_name = &addr;
//     // msg->msg_namelen = sizeof(struct sockaddr_nl);
//     // msg->msg_iov = &iov;
//     // msg->msg_iovlen = 1;

//     // if(network_netlink_write(descriptor) == fail) {
//     //     network_netlink_req_rem(req);
//     //     return nil;
//     // }

//     // return req;

// //    return // 

// //     int32_t status = sendmsg(descriptor->value, msg, 0);

// //     printf("10\n");

// //     if(status < 0) {
// //         if(errno == EAGAIN) {
// //             descriptor->status = descriptor->status & (~descriptor_state_write);
// //             return success;
// //         }
// // #ifndef   RELEASE
// //         snorlaxdbg(false, true, "warning", "fail to sendmsg => %d", errno);
// // #endif // RELEASE
// //         return fail;
// //     }



// //     return status;

// //     // BLOCKING CALL

// //     struct iovec res = { 0, };

// //     msg.msg_iov = &res;
// //     msg.msg_iovlen = 1;
// //     msg.msg_iov->iov_base = nil;
// //     msg.msg_iov->iov_len = 0;

// //     printf("11\n");

// //     int32_t len = recvmsg(descriptor->value, &msg, MSG_PEEK | MSG_TRUNC);

// //     printf("12\n");
// //     if(len < 0) {
// // #ifndef   RELEASE
// //         snorlaxdbg(false, true, "warning", "fail to recvmsg => %d", errno);
// // #endif // RELEASE
// //         return fail;
// //     }

// //     printf("3\n");

// //     char * buffer = malloc(len);

// //     msg.msg_iov->iov_base = buffer;
// //     msg.msg_iov->iov_len = len;

// //     len = recvmsg(descriptor->value, &msg, 0);

// //     printf("4\n");

// //     struct nlmsghdr * response = (struct nlmsghdr *) msg.msg_iov->iov_base;

// //     netlink_protocol_debug(stdout, response);

// //     if(response->nlmsg_type == NLMSG_ERROR) {
// //         struct nlmsgerr * err = (struct nlmsgerr *) NLMSG_DATA(response);
// //         unsigned int headerlen = sizeof(struct nlmsgerr);
        
// //         printf("error => %d\n", err->error);

// //         if(err->error == 0) {
// //             if(!(response->nlmsg_flags & NLM_F_ACK_TLVS)) {
// //                 // // 0605
// //                 // printf("nlmsgerr => %d\n", err->error);
// //                 // printf("nlmsgerr.msg.nlmsg_len => %d\n", err->msg.nlmsg_len);
// //                 // printf("nlmsgerr.msg.nlmsg_type => %d\n", err->msg.nlmsg_type);
// //                 // printf("nlmsgerr.msg.nlmsg_flags => %04x\n", err->msg.nlmsg_flags);
// //                 // printf("nlmsgerr.msg.nlmsg_seq => %04x\n", err->msg.nlmsg_seq);
// //                 // printf("nlmsgerr.msg.nlmsg_pid => %04x\n", err->msg.nlmsg_pid);\
// //                 // printf("sizeof(struct nlmsghdr) => %d\n", NLMSG_ALIGN(sizeof(struct nlmsghdr)));

// // 	// __u32		nlmsg_len;	/* Length of message including header */
// // 	// __u16		nlmsg_type;	/* Message content */
// // 	// __u16		nlmsg_flags;	/* Additional flags */
// // 	// __u32		nlmsg_seq;	/* Sequence number */
// // 	// __u32		nlmsg_pid;	/* Sending process port ID */

// //                 free(buffer);
// //                 return success;
// //             }

// //             struct nlattr * table[NLMSGERR_ATTR_MAX + 1] = {};
// //             const char * msg = NULL;

// //             /**
// //              * What is CAPPED ...
// //              */
// //             if(!(response->nlmsg_flags & NLM_F_CAPPED)) {
// //                 printf("headerlen => %d\n", headerlen);
// //                 printf("err->msg.nlmsg_len => %d\n", err->msg.nlmsg_len);
// //                 printf("NLMSG_ALIGN(sizeof(struct nlmsghdr) => %d\n", NLMSG_ALIGN(sizeof(struct nlmsghdr)));
// //                 headerlen = headerlen + err->msg.nlmsg_len - NLMSG_ALIGN(sizeof(struct nlmsghdr));
// //             }

// //             printf("parse\n");

// // //            mnl_attr_parse();


// // // struct nlattr *tb[NLMSGERR_ATTR_MAX + 1] = {};
// // // 	const struct nlmsgerr *err = mnl_nlmsg_get_payload(nlh);
// // // 	const struct nlmsghdr *err_nlh = NULL;
// // // 	unsigned int hlen = sizeof(*err);
// // // 	const char *msg = NULL;
// // // 	uint32_t off = 0;

// // // 	/* no TLVs, nothing to do here */
// // // 	if (!(nlh->nlmsg_flags & NLM_F_ACK_TLVS))
// // // 		return 0;

// // // 	/* if NLM_F_CAPPED is set then the inner err msg was capped */
// // // 	if (!(nlh->nlmsg_flags & NLM_F_CAPPED))
// // // 		hlen += mnl_nlmsg_get_payload_len(&err->msg);

// // // 	if (mnl_attr_parse(nlh, hlen, err_attr_cb, tb) != MNL_CB_OK)
// // // 		return 0;

// // // 	if (tb[NLMSGERR_ATTR_MSG])
// // // 		msg = mnl_attr_get_str(tb[NLMSGERR_ATTR_MSG]);

// // // 	if (tb[NLMSGERR_ATTR_OFFS]) {
// // // 		off = mnl_attr_get_u32(tb[NLMSGERR_ATTR_OFFS]);

// // // 		if (off > nlh->nlmsg_len) {
// // // 			fprintf(stderr,
// // // 				"Invalid offset for NLMSGERR_ATTR_OFFS\n");
// // // 			off = 0;
// // // 		} else if (!(nlh->nlmsg_flags & NLM_F_CAPPED))
// // // 			err_nlh = &err->msg;
// // // 	}

// // // 	if (tb[NLMSGERR_ATTR_MISS_TYPE])
// // // 		fprintf(stderr, "Missing required attribute type %u\n",
// // // 			mnl_attr_get_u32(tb[NLMSGERR_ATTR_MISS_TYPE]));

// // // 	if (errfn)
// // // 		return errfn(msg, off, err_nlh);

// // // 	if (msg && *msg != '\0') {
// // // 		bool is_err = !!err->error;

// // // 		print_ext_ack_msg(is_err, msg);
// // // 		return is_err ? 1 : 0;
// // // 	}


// // // header->nlmsg_len => 88
// // // header->nlmsg_type => 20
// // // header->nlmsg_flags => 0000
// // // header->nlmsg_seq => 1720645119
// // // header->nlmsg_pid => 119887

// // // 	return 0;

// //         }

// //         free(buffer);

// //         return fail;
// //     }

// //     free(buffer);

// //     printf("5\n");

// //     return success;
// }

// static int64_t network_netlink_func_write(___notnull network_netlink_t * descriptor) {
// #ifndef   RELEASE
//     snorlaxdbg(descriptor == nil, false, "critical", "");
// #endif // RELEASE
    
//     if(descriptor->value > invalid) {
//         if(descriptor->status & descriptor_state_open_out) {
//             buffer_t * out = descriptor->buffer.out;
//             int64_t total = 0;

//             for(network_netlink_message_t * node = buffer_position_get(out); node != buffer_size_get(out); node = node->next) {
//                 struct nlmsghdr * nlmsg = node->message;

//                 struct iovec iov = { .iov_base = nlmsg, .iov_len = nlmsg->nlmsg_len };
//                 struct sockaddr_nl addr = { .nl_family = AF_NETLINK };
//                 struct msghdr msg;

//                 memset(&msg, 0, sizeof(struct msghdr));

//                 msg.msg_iov = &iov;
//                 msg.msg_iovlen = 1;
//                 msg.msg_name = &addr;
//                 msg.msg_namelen = sizeof(struct sockaddr_nl);

//                 int64_t n = sendmsg(descriptor->value, &msg, 0);

//                 if(n > 0) {
//                     node->status = node->status | network_netlink_message_state_req;
                    
//                     continue;
//                 } else if(n == 0) {

//                 } else {
//                     if(errno == EAGAIN) {

//                     } else {

//                     }
//                 }
//                 break;
//             }

//             while(buffer_length(out) > 0) {
//                 struct nlmsghdr * nlmsg = (struct nlmsghdr *) buffer_front(out);

//                 struct iovec iov = { .iov_base = nlmsg, .iov_len = nlmsg->nlmsg_len };
//                 struct sockaddr_nl addr = { .nl_family = AF_NETLINK };
//                 struct msghdr msg;

//                 memset(&msg, 0, sizeof(struct msghdr));

//                 msg.msg_iov = &iov;
//                 msg.msg_iovlen = 1;
//                 msg.msg_name = &addr;
//                 msg.msg_namelen = sizeof(struct sockaddr_nl);

//                 int64_t n = sendmsg(descriptor->value, &msg, 0);

//                 if(n > 0) {
//                     continue;
//                 } else if(n == 0) {

//                 } else {
//                     if(errno == EAGAIN) {

//                     } else {

//                     }
//                 }
//             }
//         } else {
// #ifndef   RELEASE
//             snorlaxdbg(false, true, "warning", "");
// #endif // RELEASE
//         }
//     } else {
// #ifndef   RELEASE
//         snorlaxdbg(false, true, "warning", "");
// #endif // RELEASE
//     }

//     return fail;

// //     /**
// //      * REQ & RES CONCEPT ...
// //      */

// //     if(descriptor->value > invalid) {
// //         if(descriptor->status & descriptor_state_open_out) {
// //             buffer_t * out = descriptor->buffer.out;
// //             int64_t total = 0;

// //             while(sizeof(struct nlmsghdr) <= buffer_length(out)) {
// //                 struct nlmsghdr * nlmsg = (struct nlmsghdr *) buffer_front(out);
// //                 if(nlmsg->nlmsg_len <= buffer_length(out)) {
// //                     struct iovec iov = { .iov_base = nlmsg, .iov_len = nlmsg->nlmsg_len };
// //                     struct sockaddr_nl addr = { .nl_family = AF_NETLINK };
// //                     struct msghdr msg;

// //                     msg.msg_iov = &iov;
// //                     msg.msg_iovlen = 1;
// //                     msg.msg_name = &addr;
// //                     msg.msg_namelen = sizeof(struct sockaddr_nl);

// //                     int64_t n = sendmsg(descriptor->value, &msg, 0);
// //                     if(n > 0) {
// //                         buffer_position_set(out, buffer_position_get(out) + n);

// //                         descriptor->status = descriptor->status | descriptor_state_write;
// //                         total = total + n;

// //                         continue;
// //                     } else if(n == 0) {
// // #ifndef   RELEASE
// //                         snorlaxdbg(false, true, "check", "");
// // #endif // RELEASE
// //                     } else {
// //                         descriptor->status = descriptor->status & (~descriptor_state_write);
// //                         if(errno != EAGAIN) {
// // #ifndef   RELEASE
// //                             snorlaxdbg(false, true, "exception", "%d %d %p", descriptor_exception_type_system, errno, sendmsg);
// // #endif // RELEASE
// //                             descriptor_exception_set(descriptor, descriptor_exception_type_system, errno, sendmsg);

// //                             return fail;
// //                         }
// //                     }
// //                 }
// //                 break;
// //             }

// //             return total;

// //             for(struct nlmsghdr * node = buffer_front(out); ((void *) node) < ((void *) buffer_back(out)); node = ((void *) node) + node->nlmsg_len) {
// //                 struct iovec iov = { .iov_base = node, .iov_len = node->nlmsg_len };
// //                 struct sockaddr_nl addr = { .nl_family = AF_NETLINK };
// //                 struct msghdr msg;

// //                 msg.msg_iov = &iov;
// //                 msg.msg_iovlen = 1;
// //                 msg.msg_name = &addr;
// //                 msg.msg_namelen = sizeof(struct sockaddr_nl);

// //                 int64_t n = sendmsg(descriptor->value, &msg, 0);

// //                 if(n > 0) {
// //                     continue;
// //                 } else if(n == 0) {
// // #ifndef   RELEASE
// //                     snorlaxdbg(false, true, "check", "");
// // #endif // RELEASE
// //                 } else {
// //                     if(errno == EAGAIN) {
// //                         descriptor->status = descriptor->status & (~descriptor_state_write);
// //                         n = 0;
// //                     } else {
// // #ifndef   RELEASE
// //                         snorlaxdbg(false, true, "descriptor exception", "%d %d %p", descriptor_exception_type_system, errno, sendmsg);
// // #endif // RELEASE
// //                         descriptor_exception_set(descriptor, descriptor_exception_type_system, errno, sendmsg);

// //                         return fail;
// //                     }
// //                 }
// //                 break;
// //             }




// //             int64_t total = 0;
// //             buffer_t * out = descriptor->buffer.out;

// //             while(buffer_length(out) >= sizeof(struct msghdr)) {
// //                 struct msghdr * msg = (struct msghdr *) buffer_front(out);
// //                 int64_t n = sendmsg(descriptor->value, msg, 0);
// //                 if(n > 0) {
// //                     buffer_position_set(out, buffer_position_get(out) + sizeof(struct msghdr));
// //                     descriptor->status = descriptor->status | descriptor_state_write;
// //                     total = total + n;
// //                     descriptor->transfer = descriptor->transfer + n;
// //                     for(network_netlink_req_t * node = descriptor->requests->last ? descriptor->requests->last->next : descriptor->requests->head; node != nil; node = node->next) {
// //                         if(node->end <= descriptor->transfer) {
// //                             // TODO: MODIFY VARIABLE NAME: CALLBACK
// //                             if(node->callback) node->callback(node->message, node->messagelen, nil, 0);
// //                             descriptor->requests->last = node;
// //                             continue;
// //                         }
// //                         break;
// //                     }
// //                     // TODO: IMPLEMENT BUFFER RESET ... ?
// //                     continue;
// //                 } else if(n == 0) {
// // #ifndef   RELEASE
// //                     snorlaxdbg(false, true, "check", "");
// // #endif // RELEASE
// //                 } else {
// //                     if(errno == EAGAIN) {
// //                         descriptor->status = descriptor->status & (~descriptor_state_write);
// //                     } else {
// // #ifndef   RELEASE
// //                         snorlaxdbg(false, true, "descriptor exception", "%d %d %p", descriptor_exception_type_system, errno, write);
// // #endif // RELEASE
// //                         descriptor_exception_set(descriptor, descriptor_exception_type_system, errno, sendmsg);
// //                     }
// //                 }
// //                 break;
// //             }

// //             return total;
// //         } else {
// // #ifndef   RELEASE
// //             snorlaxdbg(false, true, "warning", "descriptor state not open");
// // #endif // RELEASE
// //         }
// //     } else {
// // #ifndef   RELEASE
// //         snorlaxdbg(false, true, "warning", "descriptor is not valid");
// // #endif // RELEASE
// //     }

// //     return fail;
// }

extern void netlink_protocol_debug(FILE * stream, void * data) {
#ifndef   RELEASE
    snorlaxdbg(stream == nil, false, "critical", "");
    snorlaxdbg(data == nil, false, "critical", "");
#endif // RELEASE
    struct nlmsghdr * header = (struct nlmsghdr *) data;

    fprintf(stream, "header->nlmsg_len => %u\n", header->nlmsg_len);        // length of message including header
    fprintf(stream, "header->nlmsg_type => %u\n", header->nlmsg_type);      // message content
    fprintf(stream, "header->nlmsg_flags => %04x\n", header->nlmsg_flags);  // additional flags
    fprintf(stream, "header->nlmsg_seq => %u\n", header->nlmsg_seq);        // sequence number
    fprintf(stream, "header->nlmsg_pid => %u\n", header->nlmsg_pid);        // sending process port id

//    hexdump(stream, NLMSG_DATA(header), header->nlmsg_len - NLMSG_HDRLEN);

    if(header->nlmsg_type == NLMSG_ERROR) {
        struct nlmsgerr * e = (struct nlmsgerr *) NLMSG_DATA(header);

        fprintf(stream, "e->error => %d\n", e->error);
        fprintf(stream, "e->msg.nlmsg_len => %d\n", e->msg.nlmsg_len);
        fprintf(stream, "e->msg.nlmsg_type => %d\n", e->msg.nlmsg_type);
        fprintf(stream, "e->msg.nlmsg_flags => %d\n", e->msg.nlmsg_flags);
        fprintf(stream, "e->msg.nlmsg_seq => %d\n", e->msg.nlmsg_seq);
        fprintf(stream, "e->msg.nlmsg_pid => %d\n", e->msg.nlmsg_pid);
	// int		error;
	// struct nlmsghdr msg;
    // 	__u32		nlmsg_len;	/* Length of message including header */
	// __u16		nlmsg_type;	/* Message content */
	// __u16		nlmsg_flags;	/* Additional flags */
	// __u32		nlmsg_seq;	/* Sequence number */
	// __u32		nlmsg_pid;	/* Sending process port ID */
        // fprintf(stream, "implement this");

    } else if(header->nlmsg_type == RTM_NEWADDR) {
        struct ifaddrmsg * message = NLMSG_DATA(header);
        fprintf(stream, "message->ifa_family => %d\n", message->ifa_family);
        fprintf(stream, "message->ifa_prefixlen => %d\n", message->ifa_prefixlen);
        fprintf(stream, "message->ifa_flags => %02x\n", message->ifa_flags);
        fprintf(stream, "message->ifa_scope => %d\n", message->ifa_scope);
        fprintf(stream, "message->ifa_index => %d\n", message->ifa_index);

        for(struct rtattr * attr = netlink_protocol_data_get(struct rtattr *, message, sizeof(struct ifaddrmsg)); (void *) attr < netlink_protocol_data_end(header); attr = netlink_protocol_attr_next(attr)) {
            if(attr->rta_type == IFA_ADDRESS) {
                uint8_t * value = (uint8_t *) RTA_DATA(attr);
                fprintf(stream, "address => %d.%d.%d.%d\n", value[0], value[1], value[2], value[3]);
            } else if(attr->rta_type == IFA_LOCAL) {
                uint8_t * value = (uint8_t *) RTA_DATA(attr);
                fprintf(stream, "local => %d.%d.%d.%d\n", value[0], value[1], value[2], value[3]);
            } else if(attr->rta_type == IFA_BROADCAST) {
                uint8_t * value = (uint8_t *) RTA_DATA(attr);
                fprintf(stream, "broadcast => %d.%d.%d.%d\n", value[0], value[1], value[2], value[3]);
            } else if(attr->rta_type == IFA_LABEL) {
                char * value = (char *) RTA_DATA(attr);
                fprintf(stream, "label => %s\n", value);
            } else if(attr->rta_type == IFA_FLAGS) {
                // 00000080
                uint32_t * value = (uint32_t *) RTA_DATA(attr);
                fprintf(stream, "flags => %08x\n", *value);
            } else if(attr->rta_type == IFA_CACHEINFO) {
                // HOW TO DESERIALIZE CACHE INFO
                struct ifa_cacheinfo * cacheinfo = (struct ifa_cacheinfo *) RTA_DATA(attr);
                fprintf(stream, "cacheinfo->ifa_prefered => %d\n", cacheinfo->ifa_prefered == 0xFFFFFFFF ? 1 : 0);
                fprintf(stream, "cacheinfo->ifa_valid => %d\n", cacheinfo->ifa_valid == 0xFFFFFFFF ? 1 : 0);
                fprintf(stream, "cacheinfo->cstamp => %d\n", cacheinfo->cstamp);
                fprintf(stream, "cacheinfo->tstamp => %d\n", cacheinfo->tstamp);
            } else {
#ifndef   RELEASE
                snorlaxdbg(true, false, "implement", "type => %d", attr->rta_type);
#endif // RELEASE
            }
        }
    }    
}

// static void network_netlink_func_flush(___notnull network_netlink_t * descriptor, network_netlink_is_flushed_t check) {
// #ifndef   RELEASE
//     snorlaxdbg(descriptor == nil, false, "critical", "");
//     snorlaxdbg(descriptor->value <= invalid, false, "critical", "");
// #endif // RELEASE

//     network_netlink_write(descriptor);
//     int32_t cancel = false;
//     struct pollfd descriptors;
//     descriptors.fd = descriptor->value;

//     while(cancel == false) {
//         descriptors.events = (POLLIN | POLLERR | POLLHUP | POLLNVAL);
//         if(buffer_length(descriptor->buffer.out) > 0) {
//             descriptors.events = descriptors.events | POLLOUT;
//         }
//         descriptors.revents = 0;

//         int n = poll(&descriptors, 1, 1);
//         if(n >= 0) {
//             for(int i = 0; i < 1; i++) {
//                 if(descriptors.revents & (POLLERR | POLLHUP | POLLNVAL)) {
//                     descriptor_exception_set(descriptor, descriptor_exception_type_system, descriptors.revents, poll);
//                     cancel = true;
//                     break;
//                 }
//                 if(descriptors.revents & POLLOUT) {
//                     network_netlink_write(descriptor);
//                     if(descriptor_exception_get(descriptor)) {
//                         cancel = true;
//                         break;
//                     }
//                 }
//                 if(descriptors.revents & POLLIN) {
//                     network_netlink_read(descriptor);
//                     if(descriptor_exception_get(descriptor)) {
//                         cancel = true;
//                         break;
//                     }
//                 }
//                 if(check) {
//                     if(check(descriptor) == true) {
// #ifndef   RELEASE
//                         snorlaxdbg(false, true, "debug", "flushed");
// #endif // RELEASE
//                         break;
//                     }
//                     continue;
//                 } else if(buffer_length(descriptor->buffer.out) > 0) {
//                     continue;
//                 }
//                 break;
//             }
//         }
//     }
// }

// static int64_t network_netlink_func_read(___notnull network_netlink_t * descriptor) {
// #ifndef   RELEASE
//     snorlaxdbg(descriptor == nil, false, "critical", "");
// #endif // RELEASE

//     if(descriptor->value > invalid) {
//         if(descriptor->status & descriptor_state_open_in) {
// //             int64_t total = 0;
// //             buffer_t * in = descriptor->buffer.in;

// //             while(true) {
// //                 if(buffer_remain(in) <= sizeof(struct msghdr)) {
// //                     buffer_capacity_set(in, buffer_size_get(in) + sizeof(struct msghdr) * 8);
// //                 }
// //                 struct msghdr * msg = (struct msghdr *) buffer_back(in);
// //                 int64_t n = recvmsg(descriptor->value, msg, 0);
// //                 if(n > 0) {
// //                     buffer_size_set(in, buffer_size_get(in) + sizeof(struct msghdr));

// //                     descriptor->status = descriptor->status | descriptor_state_read;
// //                     continue;
// //                 } else if(n == 0) {
// // #ifndef   RELEASE
// //                     snorlaxdbg(false, true, "descriptor exception", "%d %d %p", descriptor_exception_type_lib, descriptor_exception_no_eof, recvmsg);
// // #endif // RELEASE

// //                     descriptor_exception_set(descriptor, descriptor_exception_type_lib, descriptor_exception_no_eof, recvmsg);

// //                     total = fail;
// //                 } else {
// //                     if(errno == EAGAIN) {
// //                         total = 0;
// //                     } else {
// // #ifndef   RELEASE
// //                         snorlaxdbg(false, true, "descriptor exception", "%d %d %p", descriptor_exception_type_system, errno, recvmsg);
// // #endif // RELEASE
// //                         descriptor_exception_set(descriptor, descriptor_exception_type_system, errno, recvmsg);
// //                     }
// //                 }
// //                 descriptor->status = descriptor->status & (~descriptor_state_read);
// //                 return total;
// //             }

// //             return total;
//         } else {
// #ifndef   RELEASE
//             snorlaxdbg(false, true, "warning", "descriptor is not open");
// #endif // RELEASE
//         }
//     } else {
// #ifndef   RELEASE
//         snorlaxdbg(false, true, "warning", "descriptor is not valid");
// #endif // RELEASE
//     }

//     return fail;
// }

// static int32_t network_netlink_func_wait(___notnull network_netlink_t * descriptor, network_netlink_req_t * req) {
// #ifndef   RELEASE
//     snorlaxdbg(descriptor == nil, false, "critical", "");
// #endif // RELEASE

//     buffer_t * in = descriptor->buffer.in;
//     buffer_t * out = descriptor->buffer.out;

//     return fail;
// }
