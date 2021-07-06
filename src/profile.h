/*
 * Insert copyright here
 */

#ifdef WITH_HW_COUNTERS
extern void pmc_profile_setup(const char *side);
extern void _pmc_profile_dump(void);
extern void _pmc_profile_start(void);
extern void _pmc_profile_stop(void);
extern void _pmc_iteration_start(void);
extern void _pmc_iteration_stop(void);

#define pmc_profile_dump() do {			\
  if (pmc_profile_enabled)			\
    _pmc_profile_dump();			\
  } while (0)

#define pmc_profile_start() do {		\
  if (pmc_profile_enabled)			\
    _pmc_profile_start();			\
  } while (0)

#define pmc_profile_stop() do {			\
  if (pmc_profile_enabled)			\
    _pmc_profile_stop();			\
  } while (0)
#else
#define pmc_profile_setup(side)
#define pmc_profile_dump()
#define pmc_profile_start()
#define pmc_profile_stop()
#endif
