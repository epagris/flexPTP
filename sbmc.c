#include <flexptp/ptp_types.h>
#include <flexptp/sbmc.h>

#define COMPARE_AND_RETURN(pmp1,pmp2,field) {\
	if (pmp1->field < pmp2->field) {\
		return 0;\
	} else if (pmp1->field > pmp2->field) {\
		return 1;\
	}\
}

int ptp_select_better_master(PtpMasterProperties * pMP1, PtpMasterProperties * pMP2)
{
    COMPARE_AND_RETURN(pMP1, pMP2, priority1);
    COMPARE_AND_RETURN(pMP1, pMP2, grandmasterClockClass);
    COMPARE_AND_RETURN(pMP1, pMP2, grandmasterClockAccuracy);
    COMPARE_AND_RETURN(pMP1, pMP2, grandmasterClockVariance);
    COMPARE_AND_RETURN(pMP1, pMP2, priority2);
    COMPARE_AND_RETURN(pMP1, pMP2, grandmasterClockIdentity);
    return 1;
}
