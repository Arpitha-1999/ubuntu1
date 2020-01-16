// SPDX-License-Identifier: GPL-2.0
/*
 * hibernate.c:  Hibernaton support specific for sparc64.
 *
 * Copyright (C) 2013 Kirill V Tkhai (tkhai@yandex.ru)
 */

#include <linux/mm.h>

#include <asm/hibernate.h>
#include <asm/visasm.h>
#include <asm/page.h>
#include <asm/sections.h>
#include <asm/tlb.h>

struct saved_context saved_context;

/*
 *	pfn_is_yessave - check if given pfn is in the 'yessave' section
 */

int pfn_is_yessave(unsigned long pfn)
{
	unsigned long yessave_begin_pfn = PFN_DOWN((unsigned long)&__yessave_begin);
	unsigned long yessave_end_pfn = PFN_DOWN((unsigned long)&__yessave_end);

	return (pfn >= yessave_begin_pfn) && (pfn < yessave_end_pfn);
}

void save_processor_state(void)
{
	save_and_clear_fpu();
}

void restore_processor_state(void)
{
	struct mm_struct *mm = current->active_mm;

	tsb_context_switch_ctx(mm, CTX_HWBITS(mm->context));
}
