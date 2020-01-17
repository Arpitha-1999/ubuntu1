/* SPDX-License-Identifier: GPL-2.0 WITH Linux-syscall-yeste */
#ifndef __LINUX_RAW_H
#define __LINUX_RAW_H

#include <linux/types.h>

#define RAW_SETBIND	_IO( 0xac, 0 )
#define RAW_GETBIND	_IO( 0xac, 1 )

struct raw_config_request 
{
	int	raw_miyesr;
	__u64	block_major;
	__u64	block_miyesr;
};

#define MAX_RAW_MINORS CONFIG_MAX_RAW_DEVS

#endif /* __LINUX_RAW_H */
