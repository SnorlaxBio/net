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
#include "netlink/message/request.h"
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
static network_netlink_message_request_t * network_netlink_func_req(___notnull network_netlink_t * descriptor, struct nlmsghdr * message, network_netlink_message_request_on_t on);
static int32_t network_netlink_func_wait(___notnull network_netlink_t * descriptor, ___notnull network_netlink_message_request_t * request);

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

    printf("network netlink read\n");

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
                        node->status = node->status | network_netlink_message_state_done;
                    } else if(message->nlmsg_flags & NLM_F_MULTI && message->nlmsg_type == NLMSG_DONE) {
                        node->status = node->status | network_netlink_message_state_done;
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
            printf("netlink read => %ld\n", n);
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
#ifndef   RELEASE
                // netlink_protocol_debug(stdout, node->message);
#endif // RELEASE
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

static network_netlink_message_request_t * network_netlink_func_req(___notnull network_netlink_t * descriptor, struct nlmsghdr * message, network_netlink_message_request_on_t on) {
#ifndef   RELEASE
    snorlaxdbg(descriptor == nil, false, "critical", "");
    snorlaxdbg(message == nil, false, "critical", ""); 
#endif // RELEASE

    network_netlink_message_request_t * node = network_netlink_message_request_gen(nil, on);

    node->message = message;

    buffer_list_push(descriptor->buffer.out, (buffer_list_node_t *) node);

    if(descriptor->buffer.out->front == nil) descriptor->buffer.out->front = (buffer_list_node_t *) node;

    network_netlink_write(descriptor);

    // TODO: ERROR HANDLING

    return node;
}

static int32_t network_netlink_func_wait(___notnull network_netlink_t * descriptor, ___notnull network_netlink_message_request_t * request) {
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
                    for(network_netlink_message_t * node = (network_netlink_message_t *) in->head; node != nil; ) {
                        if(request->message->nlmsg_seq == node->message->nlmsg_seq) {
#ifndef   RELEASE
                            // netlink_protocol_debug(stdout, node->message);
#endif // RELEASE
                            
                            network_netlink_message_t * response = node;
                            node = node->next;

                            buffer_list_del(in, (buffer_list_node_t *) response);
                            buffer_list_push(request->responses, (buffer_list_node_t *) response);

                            if(response->status & network_netlink_message_state_done) {
#ifndef   RELEASE
                                snorlaxdbg(false, true, "response", "");
#endif // RELEASE
                                if(request->on) {
                                    request->on(request, network_netlink_message_state_done);
                                }
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

extern void netlink_protocol_debug(FILE * stream, void * data) {
#ifndef   RELEASE
    snorlaxdbg(stream == nil, false, "critical", "");
    snorlaxdbg(data == nil, false, "critical", "");
#endif // RELEASE
    struct nlmsghdr * header = (struct nlmsghdr *) data;
    fprintf(stream, "%p\n", data);
    fprintf(stream, "header->nlmsg_len => %u\n", header->nlmsg_len);        // length of message including header
    fprintf(stream, "header->nlmsg_type => %u\n", header->nlmsg_type);      // message content
    fprintf(stream, "header->nlmsg_flags => %04x\n", header->nlmsg_flags);  // additional flags
    fprintf(stream, "header->nlmsg_seq => %u\n", header->nlmsg_seq);        // sequence number
    fprintf(stream, "header->nlmsg_pid => %u\n", header->nlmsg_pid);        // sending process port id

    if(header->nlmsg_type == NLMSG_ERROR) {
        struct nlmsgerr * e = (struct nlmsgerr *) NLMSG_DATA(header);

        fprintf(stream, "e->error => %d\n", e->error);
        fprintf(stream, "e->msg.nlmsg_len => %d\n", e->msg.nlmsg_len);
        fprintf(stream, "e->msg.nlmsg_type => %d\n", e->msg.nlmsg_type);
        fprintf(stream, "e->msg.nlmsg_flags => %d\n", e->msg.nlmsg_flags);
        fprintf(stream, "e->msg.nlmsg_seq => %d\n", e->msg.nlmsg_seq);
        fprintf(stream, "e->msg.nlmsg_pid => %d\n", e->msg.nlmsg_pid);

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
    } else if(header->nlmsg_type == RTM_GETROUTE) {
        struct rtmsg * message = (struct rtmsg *) NLMSG_DATA(header);

        fprintf(stream, "message->rtm_family => %d\n", message->rtm_family);
        fprintf(stream, "message->rtm_dst_len => %d\n", message->rtm_dst_len);
        fprintf(stream, "message->rtm_src_len => %d\n", message->rtm_src_len);
        fprintf(stream, "message->rtm_tos => %d\n", message->rtm_tos);
        fprintf(stream, "message->rtm_table => %d\n", message->rtm_table);
        fprintf(stream, "message->rtm_protocol => %d\n", message->rtm_protocol);
        fprintf(stream, "message->rtm_scope => %d\n", message->rtm_scope);
        fprintf(stream, "message->rtm_type => %d\n", message->rtm_type);
        fprintf(stream, "message->rtm_flags => %d\n", message->rtm_flags);
        fprintf(stream, "sizeof(struct rtmsg) => %lu\n", sizeof(struct rtmsg));
        fprintf(stream, "sizeof(struct nlmsghdr) => %lu\n", sizeof(struct nlmsghdr));

        for(struct rtattr * attr = netlink_protocol_data_get(struct rtattr *, message, sizeof(struct rtmsg)); (void *) attr < netlink_protocol_data_end(header); attr = netlink_protocol_attr_next(attr)) {
            fprintf(stream, "%p\n", attr);
            fprintf(stream, "%p\n", netlink_protocol_data_end(header));
            fprintf(stream, "attr->rta_type => %d\n", attr->rta_type);
            fprintf(stream, "attr->rta_len => %d\n", attr->rta_len);
        }
    } else if(header->nlmsg_type == RTM_NEWROUTE) {
        struct rtmsg * message = (struct rtmsg *) NLMSG_DATA(header);

        fprintf(stream, "message->rtm_family => %d\n", message->rtm_family);
        fprintf(stream, "message->rtm_dst_len => %d\n", message->rtm_dst_len);
        fprintf(stream, "message->rtm_src_len => %d\n", message->rtm_src_len);
        fprintf(stream, "message->rtm_tos => %d\n", message->rtm_tos);
        fprintf(stream, "message->rtm_table => %d\n", message->rtm_table);
        fprintf(stream, "message->rtm_protocol => %d\n", message->rtm_protocol);
        fprintf(stream, "message->rtm_scope => %d\n", message->rtm_scope);
        fprintf(stream, "message->rtm_type => %d\n", message->rtm_type);
        fprintf(stream, "message->rtm_flags => %d\n", message->rtm_flags);
        fprintf(stream, "sizeof(struct rtmsg) => %lu\n", sizeof(struct rtmsg));
        fprintf(stream, "sizeof(struct nlmsghdr) => %lu\n", sizeof(struct nlmsghdr));

        for(struct rtattr * attr = netlink_protocol_data_get(struct rtattr *, message, sizeof(struct rtmsg)); (void *) attr < netlink_protocol_data_end(header); attr = netlink_protocol_attr_next(attr)) {
            fprintf(stream, "attr->rta_type => %d\n", attr->rta_type);
            fprintf(stream, "attr->rta_len => %d\n", attr->rta_len);

            if(attr->rta_type == RTA_TABLE) {
                uint32_t * value = (uint32_t *) RTA_DATA(attr);
                fprintf(stream, "table => %u\n", *value);
            } else if(attr->rta_type == RTA_GATEWAY) {
                uint8_t * value = (uint8_t *) RTA_DATA(attr);
                fprintf(stream, "gateway => %d.%d.%d.%d\n", value[0], value[1], value[2], value[3]);
            } else if(attr->rta_type == RTA_OIF) {
                uint32_t * value = (uint32_t *) RTA_DATA(attr);
                fprintf(stream, "oif => %u\n", *value);
            } else if(attr->rta_type == RTA_DST) {
                uint8_t * value = (uint8_t *) RTA_DATA(attr);
                fprintf(stream, "destination => %d.%d.%d.%d\n", value[0], value[1], value[2], value[3]);
            } else if(attr->rta_type == RTA_METRICS) {
                uint32_t * value = (uint32_t *) RTA_DATA(attr);
                fprintf(stream, "metrics => %u\n", *value);
            } else if(attr->rta_type == RTA_PREFSRC) {
                uint8_t * value = (uint8_t *) RTA_DATA(attr);
                fprintf(stream, "prefsrc => %d.%d.%d.%d\n", value[0], value[1], value[2], value[3]);
            } else {
#ifndef   RELEASE
                snorlaxdbg(true, false, "critical", "");
#endif // RELEASE
            }
        }
    }
}
