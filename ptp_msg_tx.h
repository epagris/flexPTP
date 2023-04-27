#ifndef FLEXPTP_SIM_PTP_MSG_TX_H
#define FLEXPTP_SIM_PTP_MSG_TX_H

#include <flexptp/ptp_types.h>

#ifdef ETHLIB
#include <etherlib/etherlib.h>
#endif

#ifdef LWIP
void ptp_transmit_init(struct udp_pcb *pPriE, struct udp_pcb *pPriG);   // initialize PTP transmitter
#elif defined(ETHLIB)
void ptp_transmit_init(cbd pPriE, cbd pPriG);
#endif

void ptp_transmit_msg(RawPtpMessage * pMsg);    // transmit PTP message
bool ptp_transmit_enqueue(const RawPtpMessage * pMsg);  // enqueue message TODO: refactor...

#endif                          //FLEXPTP_SIM_PTP_MSG_TX_H
