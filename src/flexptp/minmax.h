#ifndef FLEXPTP_MINMAX
#define FLEXPTP_MINMAX

#ifdef MIN
#undef MIN
#endif

#define MIN(a, b) (((a) < (b)) ? (a) : (b))

#ifdef MAX
#undef MAX
#endif

#define MAX(a, b) (((a) > (b)) ? (a) : (b))

#endif /* FLEXPTP_MINMAX */
