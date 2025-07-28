#include "settings_interface.h"

#include "ptp_defs.h"
#include "timeutils.h"
#include "ptp_types.h"
#include "ptp_core.h"
#include <string.h>

#include <flexptp_options.h>

///\cond 0
extern PtpCoreState gPtpCoreState;
#define S (gPtpCoreState)
///\endcond

void ptp_set_clock_offset(int32_t offset) {
    nsToTsI(&S.hwoptions.offset, offset);
}

int32_t ptp_get_clock_offset() {
    return nsI(&S.hwoptions.offset);
}

void ptp_prefer_master_clock(uint64_t clockId) {
    S.bmca.preventMasterSwitchOver = true;
    S.bmca.masterProps.grandmasterClockIdentity = clockId;
}

void ptp_unprefer_master_clock() {
    S.bmca.preventMasterSwitchOver = false;
}

uint64_t ptp_get_current_master_clock_identity() {
    return S.bmca.masterProps.grandmasterClockIdentity;
}

uint64_t ptp_get_own_clock_identity() {
    return S.hwoptions.clockIdentity;
}

void ptp_set_domain(uint8_t domain) {
    S.profile.domainNumber = domain;
}

uint8_t ptp_get_domain() {
    return S.profile.domainNumber;
}

#ifdef PTP_ADDEND_INTERFACE
void ptp_set_addend(uint32_t addend) {
    S.hwclock.addend = addend;
}

uint32_t ptp_get_addend() {
    return S.hwclock.addend;
}
#elif defined(PTP_HLT_INTERFACE)
void ptp_set_tuning(float tuning_ppb) {
    S.hwclock.tuning_ppb = tuning_ppb;
}

float ptp_get_tuning() {
    return S.hwclock.tuning_ppb;
}
#endif

PtpTransportType ptp_get_transport_type() {
    return S.profile.transportType;
}

void ptp_set_transport_type(PtpTransportType tp) {
    S.profile.transportType = tp;
}

void ptp_set_delay_mechanism(PtpDelayMechanism dm) {
    S.profile.delayMechanism = dm;
    ptp_reset(); // this is mandatory
}

PtpDelayMechanism ptp_get_delay_mechanism() {
    return S.profile.delayMechanism;
}

void ptp_set_transport_specific(PtpTransportSpecific tspec) {
    S.profile.transportSpecific = tspec;
    ptp_reset(); // this is mandatory
}

PtpTransportSpecific ptp_get_transport_specific() {
    return S.profile.transportSpecific;
}

void ptp_set_profile_flags(uint8_t flags) {
    S.profile.flags = flags;
    ptp_reset();
}

uint8_t ptp_get_profile_flags() {
    return S.profile.flags;
}

void ptp_set_tlv_chain_by_name(const char * tlvSet) {
    strncpy(S.profile.tlvSet, tlvSet, PTP_MAX_TLV_PRESET_NAME_LENGTH);
    ptp_reset();
}

const char * ptp_get_loaded_tlv_chain() {
    return S.profile.tlvSet;
}


void ptp_load_profile(const PtpProfile * pProfile) {
    S.profile = *pProfile;
    ptp_reset();
}
int8_t ptp_get_delay_req_log_period() {
    return S.profile.logDelayReqPeriod;
}

void ptp_set_delay_req_log_period(int8_t drlp) {
    S.profile.logDelayReqPeriod = drlp;
    ptp_reset();
}

int8_t ptp_get_sync_log_period() {
    return S.profile.logSyncPeriod;
}

void ptp_set_sync_log_period(int8_t slp) {
    S.profile.logSyncPeriod = slp;
    ptp_reset();
}

int8_t ptp_get_announce_log_period() {
    return S.profile.logAnnouncePeriod;
}

void ptp_set_announce_log_period(int8_t alp) {
    S.profile.logAnnouncePeriod = alp;
    ptp_reset();
}

void ptp_set_coarse_threshold(uint64_t ns) {
    S.slave.coarseLimit = ns;
}

uint64_t ptp_get_coarse_threshold() {
    return S.slave.coarseLimit;
}

void ptp_set_priority1(uint8_t p1) {
    S.capabilities.priority1 = p1;
    ptp_reset();
}

uint8_t ptp_get_priority1() {
    return S.capabilities.priority1;
}

void ptp_set_priority2(uint8_t p2) {
    S.capabilities.priority2 = p2;
    ptp_reset();
}

uint8_t ptp_get_priority2() {
    return S.capabilities.priority2;
}

void ptp_set_clock_class(PtpClockClass cc) {
    S.capabilities.grandmasterClockClass = cc;
}

PtpClockClass ptp_get_clock_class() {
    return S.capabilities.grandmasterClockClass;
}

void ptp_set_clock_accuracy(PtpClockAccuracy ca) {
    S.capabilities.grandmasterClockAccuracy = ca;
}

PtpClockAccuracy ptp_get_clock_accuracy() {
    return S.capabilities.grandmasterClockAccuracy;
}

void ptp_set_clock_variance(uint16_t var) {
    S.capabilities.grandmasterClockVariance = var;
}

uint16_t ptp_get_clock_variance() {
    return S.capabilities.grandmasterClockVariance;
}

void ptp_set_local_steps_removed(uint16_t lsr) {
    S.capabilities.localStepsRemoved = lsr;
}

uint16_t ptp_get_local_steps_removed() {
    return S.capabilities.localStepsRemoved;
}

void ptp_time(TimestampU * pT) {
    PTP_HW_GET_TIME(pT);
}

void ptp_set_time(TimestampU * pT) {
    PTP_SET_CLOCK(pT->sec, pT->nanosec);
}

void ptp_update_time(TimestampI * dt) {
    TimestampU tu;
    ptp_time(&tu);
    TimestampI ti;
    tsUToI(&ti, &tu);
    addTime(&ti, &ti, dt);
    tsIToU(&tu, &ti);
    ptp_set_time(&tu);
}
