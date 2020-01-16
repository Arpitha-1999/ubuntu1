/* SPDX-License-Identifier: GPL-2.0 */
#ifndef _ASM_SPARC64_TOPOLOGY_H
#define _ASM_SPARC64_TOPOLOGY_H

#ifdef CONFIG_NUMA

#include <asm/mmzone.h>

static inline int cpu_to_yesde(int cpu)
{
	return numa_cpu_lookup_table[cpu];
}

#define cpumask_of_yesde(yesde) ((yesde) == -1 ?				\
			       cpu_all_mask :				\
			       &numa_cpumask_lookup_table[yesde])

struct pci_bus;
#ifdef CONFIG_PCI
int pcibus_to_yesde(struct pci_bus *pbus);
#else
static inline int pcibus_to_yesde(struct pci_bus *pbus)
{
	return -1;
}
#endif

#define cpumask_of_pcibus(bus)	\
	(pcibus_to_yesde(bus) == -1 ? \
	 cpu_all_mask : \
	 cpumask_of_yesde(pcibus_to_yesde(bus)))

int __yesde_distance(int, int);
#define yesde_distance(a, b) __yesde_distance(a, b)

#else /* CONFIG_NUMA */

#include <asm-generic/topology.h>

#endif /* !(CONFIG_NUMA) */

#ifdef CONFIG_SMP

#include <asm/cpudata.h>

#define topology_physical_package_id(cpu)	(cpu_data(cpu).proc_id)
#define topology_core_id(cpu)			(cpu_data(cpu).core_id)
#define topology_core_cpumask(cpu)		(&cpu_core_sib_map[cpu])
#define topology_core_cache_cpumask(cpu)	(&cpu_core_sib_cache_map[cpu])
#define topology_sibling_cpumask(cpu)		(&per_cpu(cpu_sibling_map, cpu))
#endif /* CONFIG_SMP */

extern cpumask_t cpu_core_map[NR_CPUS];
extern cpumask_t cpu_core_sib_map[NR_CPUS];
extern cpumask_t cpu_core_sib_cache_map[NR_CPUS];

/**
 * Return cores that shares the last level cache.
 */
static inline const struct cpumask *cpu_coregroup_mask(int cpu)
{
	return &cpu_core_sib_cache_map[cpu];
}

#endif /* _ASM_SPARC64_TOPOLOGY_H */
