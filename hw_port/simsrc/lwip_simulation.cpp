#include "lwip_simulation.h"

extern "C" {

struct netif netif_default_obj = {{0xAA, 0xBB, 0xCC, 0xDD, 0xEE, 0xFF}};
struct netif *netif_default = &netif_default_obj;

pbuf *pbuf_alloc(enum pbuf_layer layer, uint16_t length, enum pbuf_type type) {
    pbuf *p = new pbuf;
    p->payload = new uint8_t[length];
    return p;
}

void pbuf_free(pbuf *p) {
    delete[] (uint8_t *) p->payload;
    delete p;
}

}
