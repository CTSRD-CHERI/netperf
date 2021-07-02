/*
 * Insert copyright here
 */

#include <errno.h>
#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include <pmc.h>

#include "netlib.h"
#include "netsh.h"

static int pmc_logfile_handle;
static const struct pmc_cpuinfo *cpuinfo;
static pmc_id_t *cpu_pmc;
static int pmc_num_handles;

/* RISCV counters counters */
static const char *cheri_riscv_pmc[] =
  {
   "CYCLE",
   "INST",
   "DCACHE_READ_MISS",
   "DCACHE_WRITE_MISS",
   NULL
  };

#define PMC_HANDLE(cpu, index)			\
  &cpu_pmc[cpu * cpuinfo->pm_npmc + index]

static void
pmc_cpu_setup(int cpuid)
{
  int index = 0;
  int error;
  const char *pmc_name;

#ifdef __riscv
  const char **arch_pmc = cheri_riscv_pmc;
#else
#error "CPU performance counters not supported by netperf"
#endif

  pmc_name = arch_pmc[0];
  while (pmc_name != NULL) {
    error = pmc_allocate(pmc_name, PMC_MODE_SC, /*flags*/0,
      cpuid, PMC_HANDLE(cpuid, index), /*count*/0);
    if (error != ENXIO) {
      fprintf(stderr, "netpmc: can not allocate PMC %s", pmc_name);
      exit(1);
      /* else, cpu at index is not active */
    } else {
      perror("netpmc: can not allocate ");
    }
    pmc_name = arch_pmc[++index];
  }
}

void
pmc_profile_dump()
{}
  
void
pmc_profile_setup()
{
  int ncpu;

  pmc_init();
  pmc_logfile_handle = open(pmc_profile_path, O_RDWR | O_CREAT);
  if (pmc_logfile_handle < 0) {
    perror("netpmc: open PMC logfile");
    exit(1);
  }

  if (pmc_cpuinfo(&cpuinfo)) {
    perror("netpmc: failed to get cpu info");
    exit(1);
  }

  pmc_num_handles = cpuinfo->pm_ncpu * cpuinfo->pm_npmc;
  cpu_pmc = malloc(pmc_num_handles * sizeof(pmc_id_t));
  if (cpu_pmc == NULL) {
    perror("netpmc: can not allocate pmc ID table");
    exit(1);
  }

  for (ncpu = 0; ncpu < cpuinfo->pm_ncpu; ncpu++) {
    pmc_cpu_setup(ncpu);
  }
    
  if (pmc_configure_logfile(pmc_logfile_handle)) {
    perror("netpmc: can not configure logfile");
    exit(1);
  }    
}

void
pmc_profile_start()
{
  int index;
  pmc_id_t pmc_id;

  for (index = 0; index < pmc_num_handles; index++) {
    pmc_id = cpu_pmc[index];
    if (pmc_id != PMC_ID_INVALID && pmc_start(pmc_id)) {
      perror("netpmc: can not start pmc");
      exit(1);
    }
  }
}

void
pmc_profile_stop()
{
  int index;
  pmc_id_t pmc_id;

  for (index = 0; index < pmc_num_handles; index++) {
    pmc_id = cpu_pmc[index];
    if (pmc_id != PMC_ID_INVALID && pmc_stop(pmc_id)) {
      perror("netpmc: can not start pmc");
      exit(1);
    }
  }
}
