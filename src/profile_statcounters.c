/*
 * Insert copyright here
 */

#include <signal.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>

#include <unistd.h>
#include <sys/param.h>
#include <sys/sysctl.h>
#include <sys/thr.h>
#include <machine/cheri.h>
#include <machine/cpufunc.h>
#include <machine/sysarch.h>

#include "netlib.h"
#include "netsh.h"
#include "statcounters.h"

#define MAXPRINTLEN 512
static FILE *statfile;
static statcounters_bank_t start, stop;
static statcounters_bank_t scratch;
static const char *benchmark_progname;
static char benchmark_archname[MAXPRINTLEN];
static const char *benchmark_kernabi;
/* Run qemu tracing NOPS at benchmark boundaries */
static bool profile_qtrace = false;
static bool profile_thread_qtrace = false;

#define PMC_SET_QEMU "qemu"
#define PMC_SET_QEMU_THREAD "qemu-thread"

void
pmc_profile_setup(const char *side)
{
  size_t maxlen;
  const char *netperf_abi;
  const char *test_end_condition;
  int test_end_value;
  int tmp;
  bool qemu_perthread = false;

  if (!pmc_profile_enabled)
    return;

  /* Resolve performance counters sets to trace */
  if (strcmp(PMC_SET_QEMU, pmc_profile_setname) == 0)
    profile_qtrace = true;
  else if (strcmp(PMC_SET_QEMU_THREAD, pmc_profile_setname) == 0)
    profile_thread_qtrace = profile_qtrace = true;
  /* XXX check other counter sets */

  if (profile_thread_qtrace) {
    tmp = 1;
    if (sysctlbyname("hw.qemu_trace_perthread", NULL, NULL,
                     &tmp, sizeof(tmp))) {
      perror("Can not setup qemu tracing");
      exit(1);
    }
  }
  if (profile_qtrace) {
    CHERI_START_TRACE;
    QEMU_EVENT_MARKER(0xbeef);
    CHERI_STOP_TRACE;
  }

  if (pmc_profile_path == NULL) {
    pmc_profile_path = malloc(MAXPATHLEN);
    snprintf(pmc_profile_path, MAXPATHLEN, "%s.csv", side);
  }
  statfile = fopen(pmc_profile_path, "a+");
  if (statfile == NULL) {
    perror("Can not open pmc stats file");
    exit(1);
  }

  maxlen = sizeof(benchmark_archname);
  if (sysctlbyname("hw.machine_arch", &benchmark_archname, &maxlen, NULL, 0)) {
    perror("Can not determine hw.machine_arch");
    exit(1);
  }

  benchmark_progname = side;

  statcounters_zero(&start);
  statcounters_zero(&stop);
  statcounters_zero(&scratch);
}

void
_pmc_profile_dump()
{
  statcounters_fmt_flag_t csv_fmt;

  if (ftello(statfile) == 0)
    csv_fmt = CSV_HEADER;
  else
    csv_fmt = CSV_NOHEADER;

  statcounters_diff(&scratch, &stop, &start);
  statcounters_dump_with_args(&scratch, benchmark_progname,
                              NULL, benchmark_archname,
                              statfile, csv_fmt);

  if (profile_qtrace)
    QEMU_FLUSH_TRACE_BUFFER;

  /* Reset for next iteration */
  statcounters_zero(&start);
  statcounters_zero(&stop);
  statcounters_zero(&scratch);
}

void
_pmc_profile_start()
{
  int tmp = 1;
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
    if (profile_thread_qtrace && sysarch(QEMU_SET_QTRACE, &tmp)) {
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
  }
  statcounters_sample(&start);
}

void
_pmc_profile_stop()
{
  int tmp = 0;

  statcounters_sample(&stop);
  if (profile_qtrace) {
    // See _pmc_profile_start
    if (profile_thread_qtrace && sysarch(QEMU_SET_QTRACE, &tmp)) {
      perror("Can not setup qemu tracing");
      exit(1);
    }
    CHERI_STOP_TRACE;
  }
}
