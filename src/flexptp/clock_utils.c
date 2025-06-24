#include "clock_utils.h"

#include <stdint.h>
#include <ctype.h>
#include <string.h>

#include "ptp_core.h"

#include <flexptp_options.h>

///\cond 0
#define S (gPtpCoreState)
///\endcond

// print clock identity
void ptp_print_clock_identity(uint64_t clockID) {
	uint8_t *p = (uint8_t*) &clockID;
	uint8_t i;
	for (i = 0; i < 8; i++) { // reverse byte order due to Network->Host byte order conversion
		MSG("%02x", p[7 - i]);
	}
}

// create clock identity based on MAC address
void ptp_create_clock_identity(const uint8_t * hwa) {
	uint8_t *p = (uint8_t*) &S.hwoptions.clockIdentity;
	// construct clockIdentity
	memcpy(p, hwa, 3); // first 3 octets of MAC address
	p[3] = 0xff;
	p[4] = 0xfe;
	memcpy(&p[5], &hwa[3], 3); // last 3 octets of MAC address

	// display ID
	MSG("Own clock ID: ");
	ptp_print_clock_identity(S.hwoptions.clockIdentity);
	MSG("\n");
}

// convert string clock id to 64-bit number
uint64_t hextoclkid(const char *str) {
    size_t len = strlen(str);
    uint64_t clkid = 0;
    for (size_t i = 0; i < len; i++) {
        char digit = tolower(str[i]);
        if (digit >= '0' && digit <= '9') {
            digit = digit - '0';
        } else if (digit >= 'a' && digit <= 'f') {
            digit = digit - 'a' + 10;
        } else {
            break;
        }

        clkid += (uint64_t) digit * ((uint64_t) 1 << (4 * i));
    }
    uint8_t *p = NULL;
    for (size_t i = 0; i < 8; i++) {
        p = ((uint8_t*) &clkid) + i;
        *p = ((*p & 0x0F) << 4) | ((*p & 0xF0) >> 4);
    }

    return clkid;
}
