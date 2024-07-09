#include <linux/netlink.h>
#include <linux/if_addr.h>
#include <linux/rtnetlink.h>
#include <sys/socket.h>
#include <net/if.h>
#include <arpa/inet.h>
#include <string.h>

#include "addr.h"

extern uint8_t * network_ip_addr_to_broadcast(uint8_t * in, uint32_t len, uint32_t subnetmasklen, uint8_t * out) {
    uint32_t subnetmask = 0;
    for(int32_t i = 0; i < subnetmasklen; i++) {
        subnetmask |= (1 << (31 - i));
    }

    *out = (*((uint32_t *) in) & subnetmask) | (~subnetmask);

    return out;
}

// struct network_ip_addr_req;

// typedef struct network_ip_addr_req network_ip_addr_req_t;

// struct network_ip_addr_req {
//     struct nlmsghdr header;
//     struct ifaddrmsg message;
//     char buf[256];
// };

// #define NLMSG_TAIL(req)    ((struct rtattr *) (void *) &((req)->header) + NLMSG_ALIGN((req)->header.nlmsg_len))

// /**
//  * 
//  * @param[in]       addr | const char * | 10.0.0.1/24 ... 
//  */
// extern int32_t network_ip_addr_add(uint8_t family, uint8_t * inet, uint32_t subnetmasklen, const char * dev) {
//     // TM_NEWADDR, NLM_F_CREATE|NLM_F_EXCL
//     network_ip_addr_req_t req = {
//         .header.nlmsg_len = NLMSG_LENGTH(sizeof(struct ifaddrmsg)),
//         .header.nlmsg_flags = (NLM_F_REQUEST | NLM_F_CREATE | NLM_F_EXCL),
//         .header.nlmsg_type = RTM_NEWADDR,

//         .message.ifa_family = family
//     };

//     if(family == AF_INET) {
//         struct rtattr * attr = NLMSG_TAIL(&req);

//         // IFA_LOCAL
//         attr->rta_type = IFA_LOCAL;
//         attr->rta_len = RTA_LENGTH(4);

//         memcpy(RTA_DATA(attr), inet, 4);

//         req.header.nlmsg_len = NLMSG_ALIGN(req.header.nlmsg_len) + RTA_ALIGN(4);

//         req.message.ifa_flags = 0;

//         // IFA_ADDRESS
//         attr = NLMSG_TAIL(&req);

//         attr->rta_type = IFA_ADDRESS;
//         attr->rta_len = RTA_LENGTH(4);

//         memcpy(RTA_DATA(attr), inet, 4);

//         req.header.nlmsg_len = NLMSG_ALIGN(req.header.nlmsg_len) + RTA_ALIGN(4);
//         // IFA_BROADCAST
//         uint32_t data = *((uint32_t *) inet);

//         req.message.ifa_prefixlen = subnetmasklen;
    
//         if(subnetmasklen <= 30) {
//             for(int i = 31; i >= subnetmasklen; i--) {
//                 data &= ~htonl(1 << (31 - i));
//             }

//             attr = NLMSG_TAIL(&req);

//             attr->rta_type = IFA_ADDRESS;
//             attr->rta_len = RTA_LENGTH(4);

//             memcpy(RTA_DATA(attr), &data, 4);

//             req.header.nlmsg_len = NLMSG_ALIGN(req.header.nlmsg_len) + RTA_ALIGN(4);
//         }

//         // SCOPE
//         req.message.ifa_scope = inet[0] == 127 ? RT_SCOPE_HOST : 0;
//         // INDEX
//         req.message.ifa_index = if_nametoindex(dev);
//         printf("req.message.ifa_index => %d\n", req.message.ifa_index);

//         /**
//          * SEND & RECV
//          */

//         return success;
//     } else {
// #ifndef   RELEASE
//         snorlaxdbg(true, false, "critical", "");
// #endif // RELEASE
//     }

//     return fail;
// }

// extern uint32_t network_iface_index_get(const char * name) {

// }