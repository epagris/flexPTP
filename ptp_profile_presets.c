#include <flexptp/ptp_profile_presets.h>

#define MAX_PROFILE_NAME_LENGTH (7)

enum {
    PTP_PROFILE_DEFAULT,        // default profile

    PTP_PROFILE_N
};

typedef struct {
    char name[MAX_PROFILE_NAME_LENGTH + 1];
    PtpProfile profile;         // profile object
} PtpProfilePreset;

static PtpProfilePreset sPtpProfiles[PTP_PROFILE_N] = {
    {
     "default",
     {
      PTP_TP_IPv4,
      PTP_TSPEC_UNKNOWN_DEF,
      PTP_DM_E2E,
      0,
      0}
     },
};

const PtpProfile *ptp_profile_preset_get(const char *pName)
{
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

size_t ptp_profile_preset_cnt()
{
    return PTP_PROFILE_N;
}

const char *ptp_profile_preset_get_name(size_t i)
{
    if (i < PTP_PROFILE_N) {
        return sPtpProfiles[i].name;
    } else {
        return NULL;
    }
}
