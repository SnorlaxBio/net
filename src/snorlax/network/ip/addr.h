/**
 * @file        snorlax/network/ip/addr.h
 * @brief
 * @details
 * 
 * @author      snorlax <opuntia@snorlax.bio>
 * @since       July 7, 2024
 */

#ifndef   __SNORLAX__NETWORK_IP_ADDR__H__
#define   __SNORLAX__NETWORK_IP_ADDR__H__

#include <snorlax.h>

extern uint8_t * network_ip_addr_to_broadcast(uint8_t * in, uint32_t len, uint32_t subnetmasklen, uint32_t * out);

// sudo ip addr add 10.0.0.1/24 dev tun0
//                  [address]/[subnet mask number]
// extern int32_t network_ip_addr_add(uint8_t family, uint8_t * inet, uint32_t subnetmasklen, const char * dev);
// extern uint32_t network_iface_index_get(const char * name);

#endif // __SNORLAX__NETWORK_IP_ADDR__H__
