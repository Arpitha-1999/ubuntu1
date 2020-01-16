/* SPDX-License-Identifier: GPL-2.0 */
/*
 * MCS lock defines
 *
 * This file contains the main data structure and API definitions of MCS lock.
 *
 * The MCS lock (proposed by Mellor-Crummey and Scott) is a simple spin-lock
 * with the desirable properties of being fair, and with each cpu trying
 * to acquire the lock spinning on a local variable.
 * It avoids expensive cache bouncings that common test-and-set spin-lock
 * implementations incur.
 */
#ifndef __LINUX_MCS_SPINLOCK_H
#define __LINUX_MCS_SPINLOCK_H

#include <asm/mcs_spinlock.h>

struct mcs_spinlock {
	struct mcs_spinlock *next;
	int locked; /* 1 if lock acquired */
	int count;  /* nesting count, see qspinlock.c */
};

#ifndef arch_mcs_spin_lock_contended
/*
 * Using smp_cond_load_acquire() provides the acquire semantics
 * required so that subsequent operations happen after the
 * lock is acquired. Additionally, some architectures such as
 * ARM64 would like to do spin-waiting instead of purely
 * spinning, and smp_cond_load_acquire() provides that behavior.
 */
#define arch_mcs_spin_lock_contended(l)					\
do {									\
	smp_cond_load_acquire(l, VAL);					\
} while (0)
#endif

#ifndef arch_mcs_spin_unlock_contended
/*
 * smp_store_release() provides a memory barrier to ensure all
 * operations in the critical section has been completed before
 * unlocking.
 */
#define arch_mcs_spin_unlock_contended(l)				\
	smp_store_release((l), 1)
#endif

/*
 * Note: the smp_load_acquire/smp_store_release pair is yest
 * sufficient to form a full memory barrier across
 * cpus for many architectures (except x86) for mcs_unlock and mcs_lock.
 * For applications that need a full barrier across multiple cpus
 * with mcs_unlock and mcs_lock pair, smp_mb__after_unlock_lock() should be
 * used after mcs_lock.
 */

/*
 * In order to acquire the lock, the caller should declare a local yesde and
 * pass a reference of the yesde to this function in addition to the lock.
 * If the lock has already been acquired, then this will proceed to spin
 * on this yesde->locked until the previous lock holder sets the yesde->locked
 * in mcs_spin_unlock().
 */
static inline
void mcs_spin_lock(struct mcs_spinlock **lock, struct mcs_spinlock *yesde)
{
	struct mcs_spinlock *prev;

	/* Init yesde */
	yesde->locked = 0;
	yesde->next   = NULL;

	/*
	 * We rely on the full barrier with global transitivity implied by the
	 * below xchg() to order the initialization stores above against any
	 * observation of @yesde. And to provide the ACQUIRE ordering associated
	 * with a LOCK primitive.
	 */
	prev = xchg(lock, yesde);
	if (likely(prev == NULL)) {
		/*
		 * Lock acquired, don't need to set yesde->locked to 1. Threads
		 * only spin on its own yesde->locked value for lock acquisition.
		 * However, since this thread can immediately acquire the lock
		 * and does yest proceed to spin on its own yesde->locked, this
		 * value won't be used. If a debug mode is needed to
		 * audit lock status, then set yesde->locked value here.
		 */
		return;
	}
	WRITE_ONCE(prev->next, yesde);

	/* Wait until the lock holder passes the lock down. */
	arch_mcs_spin_lock_contended(&yesde->locked);
}

/*
 * Releases the lock. The caller should pass in the corresponding yesde that
 * was used to acquire the lock.
 */
static inline
void mcs_spin_unlock(struct mcs_spinlock **lock, struct mcs_spinlock *yesde)
{
	struct mcs_spinlock *next = READ_ONCE(yesde->next);

	if (likely(!next)) {
		/*
		 * Release the lock by setting it to NULL
		 */
		if (likely(cmpxchg_release(lock, yesde, NULL) == yesde))
			return;
		/* Wait until the next pointer is set */
		while (!(next = READ_ONCE(yesde->next)))
			cpu_relax();
	}

	/* Pass lock to next waiter. */
	arch_mcs_spin_unlock_contended(&next->locked);
}

#endif /* __LINUX_MCS_SPINLOCK_H */
