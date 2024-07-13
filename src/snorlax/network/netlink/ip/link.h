/**
 * @file        snorlax/network/netlink/ip/link.h
 * @brief
 * @details
 * 
 * @author      snorlax <ceo@snorlax.bio>
 * @since       July 9, 2024
 */

#ifndef   __SNORLAX__NETWORK_NETLINK_IP_LINK__H__
#define   __SNORLAX__NETWORK_NETLINK_IP_LINK__H__

#include <snorlax.h>
#include <snorlax/network/netlink.h>
#include <snorlax/network/netlink/req.h>

//  sudo ip link set up dev tun0

typedef int64_t (*network_network_res_t)(void *);

extern network_netlink_req_t * network_netlink_ip_link_setup_req(network_netlink_t * descriptor, const char * dev, network_netlink_res_callback_t callback);
extern void network_netlink_ip_link_setup_res(void * request, uint64_t requestlen, void * response, uint64_t responselen);


#endif // __SNORLAX__NETWORK_NETLINK_IP_LINK__H__
