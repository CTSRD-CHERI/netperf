/*
 * Insert copyright here
 */

#include <stdlib.h>
#include <stdio.h>

#include <sys/param.h>
#include <sys/sysctl.h>

#include "netlib.h"
#include "netsh.h"
#include "statcounters.h"

#define MAXPRINTLEN 512
static FILE *statfile;
static statcounters_bank_t start;
static statcounters_bank_t stop;
static statcounters_bank_t cpu_statcounters;
static char benchmark_progname[MAXPRINTLEN];
static char benchmark_archname[MAXPRINTLEN];
static const char *benchmark_kernabi;

void
pmc_profile_dump()
{
	statcounters_diff(&cpu_statcounters, &stop, &start);
	statcounters_dump_with_args(&cpu_statcounters, benchmark_progname,
	    NULL, benchmark_archname,
	    statfile, CSV_HEADER);

	/* Reset for next iteration */
	statcounters_zero(&start);
	statcounters_zero(&stop);
	statcounters_zero(&cpu_statcounters);
}

void
pmc_profile_setup(const char *side)
{
	size_t maxlen;
	int cheri_kernel;
	const char *netperf_abi;
	const char *test_end_condition;
	int test_end_value;

	if (!pmc_profile_enabled)
		return;

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

	maxlen = sizeof(cheri_kernel);
	if (sysctlbyname("kern.features.cheri_kernel", &cheri_kernel,
	    &maxlen, NULL, 0)) {
		benchmark_kernabi = "hybrid";
	} else {
		benchmark_kernabi = "purecap";
	}
#ifdef __CHERI_PURE_CAPABILITY__
	netperf_abi = "purecap";
#else
	netperf_abi = "hybrid";
#endif
	snprintf(benchmark_progname, MAXPRINTLEN, "%s-%s:%s",
		 side, netperf_abi, benchmark_kernabi);

	statcounters_zero(&start);
	statcounters_zero(&stop);
	statcounters_zero(&cpu_statcounters);
}

void
pmc_profile_start()
{
	statcounters_sample(&start);
}

void
pmc_profile_stop()
{
	statcounters_sample(&stop);
}
