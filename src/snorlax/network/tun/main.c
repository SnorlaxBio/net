#include <snorlax/network/tun.h>
#include <snorlax/network/netlink.h>
#include <snorlax/eva.h>
#include <snorlax/protocol/internet.h>
#include <snorlax/protocol/internet/version4.h>
#include <snorlax/protocol/internet/version6.h>

network_tun_t * tun = nil;
descriptor_event_subscription_t * subscription = nil;

static void subscriptionOn(descriptor_event_subscription_t * subscription, uint32_t type, event_subscription_event_t * node){

}

static void openOn(descriptor_event_subscription_t * subscription, uint32_t type, event_subscription_event_t * node){
    
}

static void readOn(descriptor_event_subscription_t * subscription, uint32_t type, event_subscription_event_t * node) {
    printf("read\n");
    buffer_t * in = snorlax_eva_descriptor_event_subscription_buffer_in_get(subscription);
    // IS IP PACKET

//     while(buffer_length(in) > 0) {
//         uint8_t * datagram = buffer_front(in);

//         internet_protocol_debug(stdout, datagram);

//         uint8_t version = internet_protocol_version_get(datagram);
//         if(version == 4) {

//         } else if(version == 6) {
//             // uint32_t len = internet_protocol_version6_length_get(datagram);
//         } else {
// #ifndef   RELEASE
//             snorlaxdbg(true, false, "critical", "");
// #endif // RELEASE
//         }
//     }

    

    

    

    // 
// snorlax@surface:~$ sudo ip addr add 10.0.0.1/24 dev tun0
// snorlax@surface:~$ sudo ip link set up dev tun0
// 두 명령을 IP VERSION 처럼 설정하자.

    // descriptor_t * descriptor = subscription
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