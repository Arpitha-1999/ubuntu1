/* SPDX-License-Identifier: GPL-2.0 */
#ifndef _ASM_SH_TOPOLOGY_H
#define _ASM_SH_TOPOLOGY_H

#ifdef CONFIG_NUMA

#define cpu_to_yesde(cpu)	((void)(cpu),0)

#define cpumask_of_yesde(yesde)	((void)yesde, cpu_online_mask)

#define pcibus_to_yesde(bus)	((void)(bus), -1)
#define cpumask_of_pcibus(bus)	(pcibus_to_yesde(bus) == -1 ? \
					cpu_all_mask : \
					cpumask_of_yesde(pcibus_to_yesde(bus)))

#endif

#define mc_capable()    (1)

const struct cpumask *cpu_coregroup_mask(int cpu);

extern cpumask_t cpu_core_map[NR_CPUS];

#define topology_core_cpumask(cpu)	(&cpu_core_map[cpu])

#include <asm-generic/topology.h>

#endif /* _ASM_SH_TOPOLOGY_H */
