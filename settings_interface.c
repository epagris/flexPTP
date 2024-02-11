/* The flexPTP project, (C) András Wiesner, 2024 */

#include "ptp_core.h"
#include <flexptp/ptp_core.h>
#include <flexptp/settings_interface.h>

extern PtpCoreState gPtpCoreState;
#define S (gPtpCoreState)

// set PPS offset
void ptp_set_clock_offset(int32_t offset)
{
    nsToTsI(&S.hwoptions.offset, offset);
}

// get PPS offset
int32_t ptp_get_clock_offset()
{
    return nsI(&S.hwoptions.offset);
}

void ptp_prefer_master_clock(uint64_t clockId)
{
    S.sbmc.preventMasterSwitchOver = true;
    S.sbmc.masterProps.grandmasterClockIdentity = clockId;
}

void ptp_unprefer_master_clock()
{
    S.sbmc.preventMasterSwitchOver = false;
}

uint64_t ptp_get_current_master_clock_identity()
{
    return S.sbmc.masterProps.grandmasterClockIdentity;
}

uint64_t ptp_get_own_clock_identity()
{
    return S.hwoptions.clockIdentity;
}

void ptp_set_domain(uint8_t domain)
{
    S.profile.domainNumber = domain;
}

uint8_t ptp_get_domain()
{
    return S.profile.domainNumber;
}

void ptp_set_addend(uint32_t addend)
{
    S.hwclock.addend = addend;
}

uint32_t ptp_get_addend()
{
    return S.hwclock.addend;
}

PtpTransportType ptp_get_transport_type()
{
    return S.profile.transportType;
}

void ptp_time(TimestampU * pT)
{
    PTP_HW_GET_TIME(pT);
}
