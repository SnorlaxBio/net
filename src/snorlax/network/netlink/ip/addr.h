/**
 * @file        snorlax/network/netlink/ip/addr.h
 * @brief
 * @details
 * 
 * @author      snorlax <ceo@snorlax.bio>
 * @since       July 8, 2024
 */

#ifndef   __SNORLAX__NETWORK_NETLINK_IP_ADDR__H__
#define   __SNORLAX__NETWORK_NETLINK_IP_ADDR__H__

#include <snorlax.h>
#include <snorlax/network/netlink.h>

typedef int64_t (*network_network_res_t)(void *);

extern network_netlink_req_t * network_netlink_ip_addr_req(network_netlink_t * descriptor, uint8_t family, uint8_t * inet, uint32_t subnetmasklen, const char * dev, network_netlink_res_callback_t callback);
extern void network_netlink_ip_addr_res(void * request, uint64_t requestlen, void * response, uint64_t responselen);

#endif // __SNORLAX__NETWORK_NETLINK_IP_ADDR__H__
