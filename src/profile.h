/*
 * Insert copyright here
 */

#ifdef WITH_HW_COUNTERS
extern void pmc_profile_setup(const char *side);
extern void pmc_profile_dump(void);
extern void pmc_profile_start(void);
extern void pmc_profile_stop(void);
#else
#define pmc_profile_setup(side)
#define pmc_profile_dump()
#define pmc_profile_start()
#define pmc_profile_stop()
#endif
