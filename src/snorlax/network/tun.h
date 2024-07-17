/**
 * @file        snorlax/network/tun.h
 * @brief       
 * @details
 * 
 * @author      snorlax <ceo@snorlax.bio>
 * @since       July 6, 2024
 */

#ifndef   __SNORLAX__NETWORK_TUN__H__
#define   __SNORLAX__NETWORK_TUN__H__

#include <snorlax.h>
#include <snorlax/descriptor.h>

struct network_tun;
struct network_tun_func;

typedef struct network_tun network_tun_t;
typedef struct network_tun_func network_tun_func_t;

struct network_tun {
    network_tun_func_t * func;
    sync_t * sync;
    descriptor_buffer_t buffer;
    descriptor_exception_t exception;
    int32_t value;
    uint32_t status;
};

struct network_tun_func {
    network_tun_t * (*rem)(___notnull network_tun_t *);

    int32_t (*open)(___notnull network_tun_t *);
    int64_t (*read)(___notnull network_tun_t *);
    int64_t (*write)(___notnull network_tun_t *);
    int32_t (*close)(___notnull network_tun_t *);
    int32_t (*check)(___notnull network_tun_t *, uint32_t);

    // int32_t (*protect)(___notnull network_tun_t *);
};

extern network_tun_t * network_tun_gen(void);
extern int32_t network_tun_func_open(network_tun_t * descriptor);

/**
 * TODO: EXCLUDE, INCLUDE ROUTE ...
 */

#define network_tun_rem(descriptor)             ((descriptor)->func->rem(descriptor))
#define network_tun_open(descriptor)            ((descriptor)->func->open(descriptor))
#define network_tun_read(descriptor)            ((descriptor)->func->read(descriptor))
#define network_tun_write(descriptor)           ((descriptor)->func->write(descriptor))
#define network_tun_close(descriptor)           ((descriptor)->func->close(descriptor))
#define network_tun_check(descriptor, state)    ((descriptor)->func->check(descriptor))

#endif // __SNORLAX__NETWORK_TUN__H__
