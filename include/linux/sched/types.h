/* SPDX-License-Identifier: GPL-2.0 */
#ifndef _LINUX_SCHED_TYPES_H
#define _LINUX_SCHED_TYPES_H

#include <linux/types.h>

/**
 * struct task_cputime - collected CPU time counts
 * @stime:		time spent in kernel mode, in nayesseconds
 * @utime:		time spent in user mode, in nayesseconds
 * @sum_exec_runtime:	total time spent on the CPU, in nayesseconds
 *
 * This structure groups together three kinds of CPU time that are tracked for
 * threads and thread groups.  Most things considering CPU time want to group
 * these counts together and treat all three of them in parallel.
 */
struct task_cputime {
	u64				stime;
	u64				utime;
	unsigned long long		sum_exec_runtime;
};

#endif
