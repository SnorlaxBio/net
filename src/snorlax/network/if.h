/**
 * @file        snorlax/network/if.h
 * @brief
 * @details
 * 
 * @author      snorlax <ceo@snorlax.bio>
 * @since       Aug 17, 2024
 */

#ifndef   __SNORLAX__NETWORK_IF__H__
#define   __SNORLAX__NETWORK_IF__H__

#include <snorlax.h>

extern int32_t network_if_addr_get(const char * name, uint32_t * version4, uint8_t * version6);

#endif // __SNORLAX__NETWORK_IF__H__
