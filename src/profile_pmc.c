/*
 * Insert copyright here
 */

#include <errno.h>
#include <fcntl.h>
#include <pmc.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/param.h>
#include <sys/sysctl.h>
#include <sys/thr.h>
#include <unistd.h>

#include <machine/cheri.h>
#include <machine/cpufunc.h>
#include <machine/sysarch.h>

#include "netlib.h"
#include "netsh.h"

#ifndef __riscv
#error "PMC support only for CHERI RISC-V"
#endif

#define PMC_SET_QEMU "qemu"
#define PMC_SET_QEMU_THREAD "qemu-thread"
#define PMC_SET_PROFCLOCK "profclock"
#define PMC_SET_TOOOBA "toooba"

static int pmc_logfile_handle;
static const struct pmc_cpuinfo *cpuinfo;
const struct netperf_pmc *netperf_pmc_spec;
int netperf_npmc;
static pmc_id_t *pmc_state;

/* PMC counters for each set */
struct netperf_pmc {
  const char *np_name;
  enum pmc_mode np_mode;
  uint32_t np_flags;
};

static const struct netperf_pmc profclock_pmc_set[] = {
  {"CLOCK.PROF", PMC_MODE_SS, PMC_F_CALLCHAIN },
};

static const struct netperf_pmc toooba_pmc_set[] = {
  { "CYCLES", PMC_MODE_SC, 0 },
  /* { "INSTRET" }, */
  /* { "TIME" }, */
  /* { "BRANCH" }, */
  /* { "TRAP" }, */
};

void
pmc_profile_setup()
{
  const struct netperf_pmc *item;
  pmc_id_t pmc_id;
  int cpu;
  int idx;
  int err;

  if (!pmc_profile_enabled)
    return;

  pmc_init();
  pmc_logfile_handle = open(pmc_profile_path, O_RDWR | O_CREAT | O_TRUNC);
  if (pmc_logfile_handle < 0) {
    perror("netpmc: open PMC logfile");
    exit(1);
  }

  if (pmc_cpuinfo(&cpuinfo)) {
    perror("netpmc: failed to get cpu info");
    exit(1);
  }

  if (pmc_configure_logfile(pmc_logfile_handle)) {
    perror("netpmc: can not configure logfile");
    exit(1);
  }

  /* Allocate PMC group based on the pmc_profile_setname CLI option */
  if (strcmp(PMC_SET_QEMU, pmc_profile_setname) == 0) {
    perror("TODO use statcounters for now");
    exit(1);
  } else if (strcmp(PMC_SET_QEMU_THREAD, pmc_profile_setname) == 0) {
    perror("TODO use statcounters for now");
    exit(1);
  } else if (strcmp(PMC_SET_PROFCLOCK, pmc_profile_setname) == 0) {
    netperf_pmc_spec = profclock_pmc_set;
    netperf_npmc = sizeof(profclock_pmc_set) / sizeof(struct netperf_pmc);
  } else if (strcmp(PMC_SET_TOOOBA, pmc_profile_setname) == 0) {
    netperf_pmc_spec = toooba_pmc_set;
    netperf_npmc = sizeof(toooba_pmc_set) / sizeof(struct netperf_pmc);
  } else {
    perror("Invalid PMC counter setting");
    exit(1);
  }

  pmc_state = malloc(netperf_npmc * cpuinfo->pm_ncpu * sizeof(pmc_id_t));
  if (pmc_state == NULL) {
    perror("netpmc: can not allocate PMC table");
    exit(1);
  }

  for (idx = 0; idx < netperf_npmc; idx++) {
    item = &netperf_pmc_spec[idx];
    for (cpu = 0; cpu < cpuinfo->pm_ncpu; cpu++) {
      err = pmc_allocate(item->np_name, item->np_mode, item->np_flags, 0,
                         &pmc_state[cpu * netperf_npmc + idx], 0);
      if (err < 0) {
        fprintf(stderr, "netpmc: can not allocate PMC %s for CPU %d", item->np_name, cpu);
        exit(1);
      }
    }
  }

}

void
pmc_profile_teardown()
{
  if (!pmc_profile_enabled)
    return;

  pmc_flush_logfile();
  close(pmc_logfile_handle);
}

void
_pmc_profile_dump()
{}

void
_pmc_profile_start()
{
  int cpu, index, err;

  for (index = 0; index < netperf_npmc; index++) {
    for (cpu = 0; cpu < cpuinfo->pm_ncpu; cpu++) {
      err = pmc_start(pmc_state[cpu * netperf_npmc + index]);
      if (err < 0) {
        fprintf(stderr, "netpmc: could not start PMC %s on CPU %d", netperf_pmc_spec[index].np_name, cpu);
        exit(1);
      }
    }
  }
}

void
_pmc_profile_stop()
{
  int cpu, index, err;

  for (index = 0; index < netperf_npmc; index++) {
    for (cpu = 0; cpu < cpuinfo->pm_ncpu; cpu++) {
      err = pmc_stop(pmc_state[cpu * netperf_npmc + index]);
      if (err < 0) {
        fprintf(stderr, "netpmc: could not stop PMC %s on CPU %d", netperf_pmc_spec[index].np_name, cpu);
        exit(1);
      }
    }
  }
}
