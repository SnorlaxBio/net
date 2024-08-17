#include <sys/types.h>
#include <ifaddrs.h>
#include <netinet/in.h>
#include <string.h>

#include "if.h"

extern int32_t network_if_addr_get(const char * name, uint32_t * version4, uint8_t * version6) {
    struct ifaddrs * ifaddr = nil;

    struct sockaddr_in * addr4 = nil;
    struct sockaddr_in6 * addr6 = nil;

    if(getifaddrs(&ifaddr) == success) {
        for(struct ifaddrs * ifa = ifaddr; ifa != nil; ifa = ifa->ifa_next) {
            if(ifa->ifa_addr == nil) {
                continue;
            }
            if(strcmp(name, ifa->ifa_name) == 0) {
                if(ifa->ifa_addr->sa_family == AF_INET) {
                    addr4 = (struct sockaddr_in *) ifa->ifa_addr;
                    *version4 = addr4->sin_addr.s_addr;
                } else if(ifa->ifa_addr->sa_family = AF_INET6) {
                    addr6 = (struct sockaddr_in6 *) ifa->ifa_addr;
                    memcpy(version6, addr6->sin6_addr.s6_addr, 16);
                }
                if(addr4 && addr6) return success;
            }
        }
        freeifaddrs(ifaddr);
    }

    return fail;
}