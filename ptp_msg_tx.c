/* The flexPTP project, (C) András Wiesner, 2024 */

#include <flexptp/ptp_core.h>
#include <flexptp/ptp_defs.h>
#include <flexptp/ptp_msg_tx.h>
#include <flexptp/ptp_raw_msg_circbuf.h>
#include <flexptp/settings_interface.h>
#include <flexptp_options.h>

#include "FreeRTOS.h"
#include "queue.h"

static struct {
#ifdef LWIP
    struct udp_pcb *pPri_Ev;
    struct udp_pcb *pPri_Gen;

#elif defined(ETHLIB)
    cbd pPri_Ev;
    cbd pPri_Gen;

#endif
} sPcbLut = { 0 };

static const uint16_t sPortLut[2] = { PTP_PORT_EVENT, PTP_PORT_GENERAL };
static ip4_addr_t sIpLut[2] = { 0 };
static const uint8_t *sEthLut[2] = { PTP_ETHERNET_PRIMARY };

#ifdef LWIP
void ptp_transmit_init(struct udp_pcb *pPriE, struct udp_pcb *pPriG)
{
    sPcbLut.pPri_Ev = pPriE;
    sPcbLut.pPri_Gen = pPriG;

    sIpLut[0] = PTP_IGMP_PRIMARY;

}
#elif defined(ETHLIB)

void ptp_transmit_cb_handler(uint32_t ts_s, uint32_t ts_ns, uint32_t tag);      // pre-declaration

void ptp_transmit_init(cbd pPriE, cbd pPriG)
{
    sPcbLut.pPri_Ev = pPriE;
    sPcbLut.pPri_Gen = pPriG;

    sIpLut[0] = PTP_IGMP_PRIMARY;

    ts_set_tx_callback(pPriE, ptp_transmit_cb_handler);
    ts_set_tx_callback(pPriG, ptp_transmit_cb_handler);

}
#endif

// release buffer
#ifdef LWIP
void ptp_transmit_free(struct pbuf *pPBuf)
{
    pbuf_free(pPBuf);
}
#elif defined(ETHLIB)
// No need to define this...
#endif

extern PtpCircBuf gRawRxMsgBuf, gRawTxMsgBuf;

#ifdef LWIP
void ptp_transmit_cb_handler(struct pbuf *pPBuf)
{
    RawPtpMessage *pMsg = (RawPtpMessage *) pPBuf->tag;
    pMsg->ts.sec = pPBuf->time_s;
    pMsg->ts.nanosec = pPBuf->time_ns;
    if (pMsg->pTxCb) {
        pMsg->pTxCb(pMsg);
    }
    ptp_circ_buf_free(&gRawTxMsgBuf);
}
#elif defined(ETHLIB)
void ptp_transmit_cb_handler(uint32_t ts_s, uint32_t ts_ns, uint32_t tag)
{
    RawPtpMessage *pMsg = (RawPtpMessage *) tag;
    pMsg->ts.sec = ts_s;
    pMsg->ts.nanosec = ts_ns;
    pMsg->pTs->sec = ts_s;
    pMsg->pTs->nanosec = ts_ns;
    if (pMsg->pTxCb) {
        pMsg->pTxCb(pMsg);
    }
    // free buffer
    ptp_circ_buf_free(&gRawTxMsgBuf);
}
#endif

bool ptp_transmit_enqueue(const RawPtpMessage * pMsg)
{
    extern PtpCircBuf gRawTxMsgBuf;
    extern QueueHandle_t gTxPacketFIFO;
    RawPtpMessage *pMsgAlloc = ptp_circ_buf_alloc(&gRawTxMsgBuf);
    if (pMsgAlloc) {
        *pMsgAlloc = *pMsg;
        uint8_t idx = ptp_circ_buf_commit(&gRawTxMsgBuf);
        bool hptWoken = false;
        if (xPortIsInsideInterrupt()) {
            xQueueSendFromISR(gTxPacketFIFO, &idx, &hptWoken);
        } else {
            xQueueSend(gTxPacketFIFO, &idx, portMAX_DELAY);
        }
        return true;
    } else {
        MSG("enqueue failed!");
        return false;
    }
}

#ifdef LWIP
void ptp_transmit_msg(RawPtpMessage * pMsg)
{
    PtpTransportType tp = ptp_get_transport_type();
    PtpDelayMechanism dm = pMsg->tx_dm;
    PtpMessageClass mc = pMsg->tx_mc;

    // allocate buffer
    struct pbuf *txbuf = NULL;
    txbuf = pbuf_alloc(PBUF_TRANSPORT, pMsg->size, PBUF_RAM);

    // fill buffer
    memcpy(txbuf->payload, pMsg->data, pMsg->size);
    txbuf->ts_writeback_addr[0] = (uint32_t *) & (pMsg->pTs->sec);
    txbuf->ts_writeback_addr[1] = (uint32_t *) & (pMsg->pTs->nanosec);
    txbuf->tag = pMsg;
    txbuf->tx_cb = ptp_transmit_cb_handler;

    if (tp == PTP_TP_IPv4) {
        struct udp_pcb *pPcb = ((struct udp_pcb **)&sPcbLut)[2 * ((int)dm) + (int)mc];
        uint16_t port = sPortLut[(int)mc];
        ip_addr_t ipaddr = sIpLut[(int)dm];
        udp_sendto(pPcb, txbuf, &ipaddr, port);
    }

    pbuf_free(txbuf);           // release buffer
}
#elif defined(ETHLIB)
void ptp_transmit_msg(RawPtpMessage * pMsg)
{
    PtpTransportType tp = ptp_get_transport_type();
    PtpDelayMechanism dm = pMsg->tx_dm;
    PtpMessageClass mc = pMsg->tx_mc;

    if (tp == PTP_TP_IPv4) {
        cbd conn = ((cbd *) & sPcbLut)[2 * ((int)dm) + (int)mc];
        uint16_t port = sPortLut[(int)mc];
        ip_addr_t ipaddr = sIpLut[(int)dm];
        udp_sendto_arg(conn, pMsg->data, pMsg->size, ipaddr, port, (uint32_t) pMsg);
    }
}
#endif
