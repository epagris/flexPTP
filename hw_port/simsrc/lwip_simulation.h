#ifndef FLEXPTP_SIM_LWIP_SIMULATION_H
#define FLEXPTP_SIM_LWIP_SIMULATION_H

#ifdef __cplusplus
extern "C" {
#endif

#include <stdint.h>
#include <arpa/inet.h>

    typedef struct ip4_addr {
        uint32_t addr;
    } ip4_addr_t;

    typedef ip4_addr_t ip_addr_t;

#define ipaddr_addr(x) inet_addr(x)

    enum pbuf_layer {
        PBUF_TRANSPORT, PBUF_IP, PBUF_LINK, PBUF_RAW_TX,
        PBUF_RAW
    };

    enum pbuf_type {
        PBUF_RAM, PBUF_ROM, PBUF_REF, PBUF_POOL
    };

/** Main packet buffer struct */
    struct pbuf {
        void *payload;
        /*uint16_t tot_len;
           uint16_t len; */
        uint32_t time_s, time_ns;
    };

    struct pbuf *pbuf_alloc(enum pbuf_layer layer, uint16_t length, enum pbuf_type type);
    void pbuf_free(struct pbuf *p);

    struct netif {
        uint8_t hwaddr[6];
    };

    extern struct netif *netif_default;

#ifdef __cplusplus
}
#endif
#endif                          //FLEXPTP_SIM_LWIP_SIMULATION_H
