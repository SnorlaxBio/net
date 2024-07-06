#include <snorlax/network/tun.h>
#include <snorlax/eva.h>

network_tun_t * tun = nil;
descriptor_event_subscription_t * subscription = nil;

static void subscriptionOn(descriptor_event_subscription_t * subscription, uint32_t type, event_subscription_event_t * node){

}

static void openOn(descriptor_event_subscription_t * subscription, uint32_t type, event_subscription_event_t * node){
    
}

static void readOn(descriptor_event_subscription_t * subscription, uint32_t type, event_subscription_event_t * node) {

}

static void writeOn(descriptor_event_subscription_t * subscription, uint32_t type, event_subscription_event_t * node) {

}

static void closeOn(descriptor_event_subscription_t * subscription, uint32_t type, event_subscription_event_t * node) {

}

static void exceptionOn(descriptor_event_subscription_t * subscription, uint32_t type, event_subscription_event_t * node) {

}

static descriptor_event_subscription_handler_t handler[descriptor_event_type_max] = { subscriptionOn, openOn, readOn, writeOn, closeOn, exceptionOn };

int main(int argc, char ** argv) {
    snorlax_eva_on();

    tun = network_tun_gen();

    if(network_tun_open(tun) == success) {
        subscription = snorlax_eva_descriptor_sub((descriptor_t *) tun, handler);
        return snorlax_eva_run();
    }

// 이 정보에 대해서 알아볼 것
// tun0: flags=4240<POINTOPOINT,NOARP,MULTICAST>  mtu 1500
//         inet 10.0.0.1  netmask 255.255.255.0  destination 10.0.0.1
//         unspec 00-00-00-00-00-00-00-00-00-00-00-00-00-00-00-00  txqueuelen 1500  (UNSPEC)
//         RX packets 0  bytes 0 (0.0 B)
//         RX errors 0  dropped 0  overruns 0  frame 0
//         TX packets 0  bytes 0 (0.0 B)
//         TX errors 0  dropped 0 overruns 0  carrier 0  collisions 0
    
    tun = network_tun_rem(tun);

    return -1;
}