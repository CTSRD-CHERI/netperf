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
#include <sys/time.h>
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

/* Run qemu tracing NOPS at benchmark boundaries */
static bool profile_qtrace = false;
static bool profile_thread_qtrace = false;

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

static void
init_netperf_pmc(pmc_value_t initial_value)
{
  const struct netperf_pmc *item;
  int cpu;
  int idx;
  int err;

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

  pmc_state = malloc(netperf_npmc * cpuinfo->pm_ncpu * sizeof(pmc_id_t));
  if (pmc_state == NULL) {
    perror("netpmc: can not allocate PMC table");
    exit(1);
  }

  for (idx = 0; idx < netperf_npmc; idx++) {
    item = &netperf_pmc_spec[idx];
    for (cpu = 0; cpu < cpuinfo->pm_ncpu; cpu++) {
      err = pmc_allocate(item->np_name, item->np_mode, item->np_flags, 0,
                         &pmc_state[cpu * netperf_npmc + idx], initial_value);
      if (err < 0) {
        fprintf(stderr, "netpmc: can not allocate PMC %s for CPU %d\n", item->np_name, cpu);
        exit(1);
      }
    }
  }
}

void
pmc_profile_setup(const char *side)
{
  struct clockinfo clkinfo;
  size_t clkinfo_size;
  int qtrace_perthread;
  unsigned long sample_ticks;
  pmc_value_t initial_value = 0;

  if (!pmc_profile_enabled)
    return;

  pmc_init();

  /* compute sample_ticks from profclock frequency */
  clkinfo_size = sizeof(clkinfo);
  if (sysctlbyname("kern.clockrate", &clkinfo, &clkinfo_size, NULL, 0)) {
    perror("netpmc: can not read kernel clock info");
    exit(1);
  }
  /* Sample so that we take ~1000 samples per second */
  sample_ticks = clkinfo.profhz / 1000 + 1;
  if (sample_ticks == 1)
    perror("netpmc: (warning) sampling at profclock frequency limit");

  /* Allocate PMC group based on the pmc_profile_setname CLI option */
  if (strcmp(PMC_SET_QEMU, pmc_profile_setname) == 0) {
    profile_qtrace = true;
  } else if (strcmp(PMC_SET_QEMU_THREAD, pmc_profile_setname) == 0) {
    profile_qtrace = true;
    profile_thread_qtrace = true;
  } else if (strcmp(PMC_SET_PROFCLOCK, pmc_profile_setname) == 0) {
    netperf_pmc_spec = profclock_pmc_set;
    netperf_npmc = sizeof(profclock_pmc_set) / sizeof(struct netperf_pmc);
    initial_value = sample_ticks;
  } else if (strcmp(PMC_SET_TOOOBA, pmc_profile_setname) == 0) {
    netperf_pmc_spec = toooba_pmc_set;
    netperf_npmc = sizeof(toooba_pmc_set) / sizeof(struct netperf_pmc);
  } else {
    perror("Invalid PMC counter setting");
    exit(1);
  }

  if (profile_qtrace) {
    if (profile_thread_qtrace) {
      qtrace_perthread = 1;
      if (sysctlbyname("hw.qemu_trace_perthread", NULL, NULL,
                       &qtrace_perthread, sizeof(qtrace_perthread))) {
        perror("Can not setup qemu tracing");
        exit(1);
      }
    }
    /* Emit the benchmark iteration start marker */
    CHERI_START_TRACE;
    QEMU_EVENT_MARKER(0xbeef);
    CHERI_STOP_TRACE;
  } else {
    init_netperf_pmc(initial_value);
  }
}

void
pmc_profile_teardown()
{
  const struct netperf_pmc *item;
  int idx, cpu, err;

  if (!pmc_profile_enabled)
    return;

  /* Drop all PMCs */
  for (idx = 0; idx < netperf_npmc; idx++) {
    for (cpu = 0; cpu < cpuinfo->pm_ncpu; cpu++) {
      err = pmc_release(pmc_state[cpu * netperf_npmc + idx]);
      if (err < 0) {
        item = &netperf_pmc_spec[idx];
        fprintf(stderr, "Failed to release PMC %s for CPU %d\n", item->np_name, cpu);
      }
    }
  }

  close(pmc_logfile_handle);
}

void
_pmc_profile_dump()
{
  if (profile_qtrace)
    QEMU_FLUSH_TRACE_BUFFER;
  else
    pmc_flush_logfile();
}

void
_pmc_profile_start()
{
  int cpu, index, err;
  int thread_qtrace = 1;
  pid_t pid;
  long tid;

  if (profile_qtrace) {
    /* This is extra work that whould probably be avoided here */
    pid = getpid();
    thr_self(&tid);
    /*
     * Note: We use the QTRACE flag to notify the OS that it
     * should switch tracing on/off on this thread, so we need to set
     * it on profiling start and end, otherwise we
     * will pick-up extra information from the rest of the benchmark.
     * The order is relevant. We first need to set the QTRACE flag
     * and then enable tracing, otherwise we may pick up things from
     * other threads.
     */
    if (profile_thread_qtrace && sysarch(QEMU_SET_QTRACE, &thread_qtrace)) {
      perror("Can not setup qemu tracing");
      exit(1);
    }
    CHERI_START_TRACE;
    /*
     * Make sure that we use the correct context for tracing.
     * We accept the fact that if we trace perthread, we may miss
     * the first context switch if we happen to switch just after
     * the sysarch.
     */
    QEMU_EVENT_CONTEXT_UPDATE(pid, tid, -1UL);
  } else {
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
}

void
_pmc_profile_stop()
{
  int cpu, index, err;
  int thread_qtrace = 0;

  if (profile_qtrace) {
    // See _pmc_profile_start
    if (profile_thread_qtrace && sysarch(QEMU_SET_QTRACE, &thread_qtrace)) {
      perror("Can not setup qemu tracing");
      exit(1);
    }
    CHERI_STOP_TRACE;
  } else {
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
}
