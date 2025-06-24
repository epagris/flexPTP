#include "ptp_profile_presets.h"
#include "ptp_types.h"
#include "ptp_defs.h"

#include <stdint.h>
#include <string.h>

/**
 * @brief Enumeration type for predefined profiles
 */
enum PtpProfileEnum {
    PTP_PROFILE_DEFAULT, // default profile
    PTP_PROFILE_GPTP,    // gPTP (802.1AS)
    PTP_PROFILE_DEF_P2P, // default, but with P2P delay mechanism
    PTP_PROFILE_N
};

enum PtpTlvPresetEnum {
    PTP_TLVP_GPTP_ORG, // gPTP (802.1AS) TLV
    PTP_TLVP_N
};

// -------- gPTP -------------

typedef struct __attribute__((packed)) {
    PTP_TLV_HEADER
    uint8_t organizationId[3];
    uint8_t organizationSubType[3];
    uint32_t cumulativeScaleRateOffset;
    uint16_t gmTimeBaseIndicator;
    uint8_t lastGmPhaseChange[12];
    uint32_t scaledLastGmFreqChange;
} PtpGPtpOrganizationTlv;

static const PtpGPtpOrganizationTlv gPtp_OrgTlv = {
    .type = FLEXPTP_ntohs(PTP_TLV_ORGANIZATION_EXTENSION),
    .length = FLEXPTP_ntohs(28),
    .organizationId = {0x00, 0x80, 0xC2},
    .organizationSubType = {0x00, 0x00, 0x01},
    .cumulativeScaleRateOffset = FLEXPTP_ntohl(0),
    .gmTimeBaseIndicator = FLEXPTP_ntohs(0),
    .lastGmPhaseChange = {0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0},
    .scaledLastGmFreqChange = FLEXPTP_ntohl(0)};

static const PtpProfileTlvElement gPtp_TlvSet = {
    .data = &gPtp_OrgTlv,
    .size = sizeof(PtpGPtpOrganizationTlv),
    .msgType = PTP_MT_Follow_Up,
    NULL};

// ---------------------------

/**
 * @brief PTP profile preset structure.
 */
typedef struct {
    char name[PTP_MAX_PROFILE_NAME_LENGTH + 1]; ///< Name of the profile
    PtpProfile profile;                     ///< Associated profile object
} PtpProfilePreset;

/**
 * @brief PTP TLV presets
 */
typedef struct {
    char name[PTP_MAX_TLV_PRESET_NAME_LENGTH + 1]; ///< Name of the TLV preset
    const PtpProfileTlvElement *tlvList;       ///< Linked list of the TLVs
} PtpTlvPreset;

// clang-format off

/**
 * Predefined TLV chains.
 */
static PtpTlvPreset sTlvPresets[PTP_TLVP_N] = {
    {
        "gptp",
        &gPtp_TlvSet
    }
};

/**
 * Predefined profiles.
 */
static PtpProfilePreset sPtpProfiles[PTP_PROFILE_N] = {
    {"default",
        {
            PTP_TP_IPv4,
            PTP_TSPEC_UNKNOWN_DEF,
            PTP_DM_E2E,
            0,
            0,
            1,
            0,
            PTP_PF_NONE,
            ""
        }
    },
    {"gPTP",
        {
            PTP_TP_802_3,
            PTP_TSPEC_GPTP_8021AS,
            PTP_DM_P2P,
            0,
            -3,
            0,
            0,
            PTP_PF_ISSUE_SYNC_FOR_COMPLIANT_SLAVE_ONLY_IN_P2P,
            "gptp"
        }
    },
    {"defp2p",
        {
            PTP_TP_IPv4,
            PTP_TSPEC_UNKNOWN_DEF,
            PTP_DM_P2P,
            0,
            0,
            1,
            0,
            PTP_PF_NONE,
            ""
        }
    }
};

// clang-format on

const PtpProfile *ptp_profile_preset_get(const char *pName) {
    size_t i = 0;
    PtpProfile *pProfile = NULL;
    for (i = 0; i < PTP_PROFILE_N; i++) {
        if (!strcmp(sPtpProfiles[i].name, pName)) {
            pProfile = &sPtpProfiles[i].profile;
            break;
        }
    }
    return pProfile;
}

const PtpProfileTlvElement *ptp_tlv_chain_preset_get(const char *pName) {
    size_t i = 0;
    const PtpProfileTlvElement *pTlvChain = NULL;
    for (i = 0; i < PTP_TLVP_N; i++) {
        if (!strcmp(sTlvPresets[i].name, pName)) {
            pTlvChain = sTlvPresets[i].tlvList;
            break;
        }
    }
    return pTlvChain;
}

size_t ptp_profile_preset_cnt() {
    return PTP_PROFILE_N;
}

size_t ptp_tlv_chain_preset_cnt() {
    return PTP_TLVP_N;
}

const char *ptp_profile_preset_get_name(size_t i) {
    if (i < PTP_PROFILE_N) {
        return sPtpProfiles[i].name;
    } else {
        return NULL;
    }
}

const char *ptp_tlv_chain_preset_get_name(size_t i) {
    if (i < PTP_TLVP_N) {
        return sTlvPresets[i].name;
    } else {
        return NULL;
    }
}

