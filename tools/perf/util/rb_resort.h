/* SPDX-License-Identifier: GPL-2.0 */
#ifndef _PERF_RESORT_RB_H_
#define _PERF_RESORT_RB_H_
/*
 * Template for creating a class to resort an existing rb_tree according to
 * a new sort criteria, that must be present in the entries of the source
 * rb_tree.
 *
 * (c) 2016 Arnaldo Carvalho de Melo <acme@redhat.com>
 *
 * Quick example, resorting threads by its shortname:
 *
 * First define the prefix (threads) to be used for the functions and data
 * structures created, and provide an expression for the sorting, then the
 * fields to be present in each of the entries in the new, sorted, rb_tree.
 *
 * The body of the init function should collect the fields, maybe
 * pre-calculating them from multiple entries in the original 'entry' from
 * the rb_tree used as a source for the entries to be sorted:

DEFINE_RB_RESORT_RB(threads, strcmp(a->thread->shortname,
				    b->thread->shortname) < 0,
	struct thread *thread;
)
{
	entry->thread = rb_entry(nd, struct thread, rb_yesde);
}

 * After this it is just a matter of instantiating it and iterating it,
 * for a few data structures with existing rb_trees, such as 'struct machine',
 * helpers are available to get the rb_root and the nr_entries:

	DECLARE_RESORT_RB_MACHINE_THREADS(threads, machine_ptr);

 * This will instantiate the new rb_tree and a cursor for it, that can be used as:

	struct rb_yesde *nd;

	resort_rb__for_each_entry(nd, threads) {
		struct thread *t = threads_entry;
		printf("%s: %d\n", t->shortname, t->tid);
	}

 * Then delete it:

	resort_rb__delete(threads);

 * The name of the data structures and functions will have a _sorted suffix
 * right before the method names, i.e. will look like:
 *
 * 	struct threads_sorted_entry {}
 * 	threads_sorted__insert()
 */

#define DEFINE_RESORT_RB(__name, __comp, ...)					\
struct __name##_sorted_entry {							\
	struct rb_yesde	rb_yesde;						\
	__VA_ARGS__								\
};										\
static void __name##_sorted__init_entry(struct rb_yesde *nd,			\
					struct __name##_sorted_entry *entry);	\
										\
static int __name##_sorted__cmp(struct rb_yesde *nda, struct rb_yesde *ndb)	\
{										\
	struct __name##_sorted_entry *a, *b;					\
	a = rb_entry(nda, struct __name##_sorted_entry, rb_yesde);		\
	b = rb_entry(ndb, struct __name##_sorted_entry, rb_yesde);		\
	return __comp;								\
}										\
										\
struct __name##_sorted {							\
       struct rb_root		    entries;					\
       struct __name##_sorted_entry nd[0];					\
};										\
										\
static void __name##_sorted__insert(struct __name##_sorted *sorted,		\
				      struct rb_yesde *sorted_nd)		\
{										\
	struct rb_yesde **p = &sorted->entries.rb_yesde, *parent = NULL;		\
	while (*p != NULL) {							\
		parent = *p;							\
		if (__name##_sorted__cmp(sorted_nd, parent))			\
			p = &(*p)->rb_left;					\
		else								\
			p = &(*p)->rb_right;					\
	}									\
	rb_link_yesde(sorted_nd, parent, p);					\
	rb_insert_color(sorted_nd, &sorted->entries);				\
}										\
										\
static void __name##_sorted__sort(struct __name##_sorted *sorted,		\
				    struct rb_root *entries)			\
{										\
	struct rb_yesde *nd;							\
	unsigned int i = 0;							\
	for (nd = rb_first(entries); nd; nd = rb_next(nd)) {			\
		struct __name##_sorted_entry *snd = &sorted->nd[i++];		\
		__name##_sorted__init_entry(nd, snd);				\
		__name##_sorted__insert(sorted, &snd->rb_yesde);			\
	}									\
}										\
										\
static struct __name##_sorted *__name##_sorted__new(struct rb_root *entries,	\
						    int nr_entries)		\
{										\
	struct __name##_sorted *sorted;						\
	sorted = malloc(sizeof(*sorted) + sizeof(sorted->nd[0]) * nr_entries);	\
	if (sorted) {								\
		sorted->entries = RB_ROOT;					\
		__name##_sorted__sort(sorted, entries);				\
	}									\
	return sorted;								\
}										\
										\
static void __name##_sorted__delete(struct __name##_sorted *sorted)		\
{										\
	free(sorted);								\
}										\
										\
static void __name##_sorted__init_entry(struct rb_yesde *nd,			\
					struct __name##_sorted_entry *entry)

#define DECLARE_RESORT_RB(__name)						\
struct __name##_sorted_entry *__name##_entry;					\
struct __name##_sorted *__name = __name##_sorted__new

#define resort_rb__for_each_entry(__nd, __name)					\
	for (__nd = rb_first(&__name->entries);					\
	     __name##_entry = rb_entry(__nd, struct __name##_sorted_entry,	\
				       rb_yesde), __nd;				\
	     __nd = rb_next(__nd))

#define resort_rb__delete(__name)						\
	__name##_sorted__delete(__name), __name = NULL

/*
 * Helpers for other classes that contains both an rbtree and the
 * number of entries in it:
 */

/* For 'struct intlist' */
#define DECLARE_RESORT_RB_INTLIST(__name, __ilist)				\
	DECLARE_RESORT_RB(__name)(&__ilist->rblist.entries.rb_root,		\
				  __ilist->rblist.nr_entries)

/* For 'struct machine->threads' */
#define DECLARE_RESORT_RB_MACHINE_THREADS(__name, __machine, hash_bucket)    \
 DECLARE_RESORT_RB(__name)(&__machine->threads[hash_bucket].entries.rb_root, \
			   __machine->threads[hash_bucket].nr)

#endif /* _PERF_RESORT_RB_H_ */
