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
static statcounters_bank_t start, stop;
static statcounters_bank_t scratch;
static const char *benchmark_progname;
static char benchmark_archname[MAXPRINTLEN];
static const char *benchmark_kernabi;

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

	/* Reset for next iteration */
	statcounters_zero(&start);
	statcounters_zero(&stop);
	statcounters_zero(&scratch);
}

void
_pmc_profile_start()
{
	statcounters_sample(&start);
}

void
_pmc_profile_stop()
{
	statcounters_sample(&stop);
}
