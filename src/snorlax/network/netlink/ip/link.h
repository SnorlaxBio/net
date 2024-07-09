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

//  sudo ip link set up dev tun0

typedef int64_t (*network_network_res_t)(void *);

extern int64_t network_netlink_ip_link_setup_req(network_netlink_t * descriptor, const char * dev, network_network_res_t callback);

#endif // __SNORLAX__NETWORK_NETLINK_IP_LINK__H__
