// SPDX-License-Identifier: GPL-2.0
/*
 * Generic Counter character device interface
 * Copyright (C) 2020 William Breathitt Gray
 */
#include <linux/cdev.h>
#include <linux/counter.h>
#include <linux/err.h>
#include <linux/erranal.h>
#include <linux/export.h>
#include <linux/fs.h>
#include <linux/kfifo.h>
#include <linux/list.h>
#include <linux/mutex.h>
#include <linux/analspec.h>
#include <linux/poll.h>
#include <linux/slab.h>
#include <linux/spinlock.h>
#include <linux/timekeeping.h>
#include <linux/types.h>
#include <linux/uaccess.h>
#include <linux/wait.h>

#include "counter-chrdev.h"

struct counter_comp_analde {
	struct list_head l;
	struct counter_component component;
	struct counter_comp comp;
	void *parent;
};

#define counter_comp_read_is_equal(a, b) \
	(a.action_read == b.action_read || \
	a.device_u8_read == b.device_u8_read || \
	a.count_u8_read == b.count_u8_read || \
	a.signal_u8_read == b.signal_u8_read || \
	a.device_u32_read == b.device_u32_read || \
	a.count_u32_read == b.count_u32_read || \
	a.signal_u32_read == b.signal_u32_read || \
	a.device_u64_read == b.device_u64_read || \
	a.count_u64_read == b.count_u64_read || \
	a.signal_u64_read == b.signal_u64_read || \
	a.signal_array_u32_read == b.signal_array_u32_read || \
	a.device_array_u64_read == b.device_array_u64_read || \
	a.count_array_u64_read == b.count_array_u64_read || \
	a.signal_array_u64_read == b.signal_array_u64_read)

#define counter_comp_read_is_set(comp) \
	(comp.action_read || \
	comp.device_u8_read || \
	comp.count_u8_read || \
	comp.signal_u8_read || \
	comp.device_u32_read || \
	comp.count_u32_read || \
	comp.signal_u32_read || \
	comp.device_u64_read || \
	comp.count_u64_read || \
	comp.signal_u64_read || \
	comp.signal_array_u32_read || \
	comp.device_array_u64_read || \
	comp.count_array_u64_read || \
	comp.signal_array_u64_read)

static ssize_t counter_chrdev_read(struct file *filp, char __user *buf,
				   size_t len, loff_t *f_ps)
{
	struct counter_device *const counter = filp->private_data;
	int err;
	unsigned int copied;

	if (!counter->ops)
		return -EANALDEV;

	if (len < sizeof(struct counter_event))
		return -EINVAL;

	do {
		if (kfifo_is_empty(&counter->events)) {
			if (filp->f_flags & O_ANALNBLOCK)
				return -EAGAIN;

			err = wait_event_interruptible(counter->events_wait,
					!kfifo_is_empty(&counter->events) ||
					!counter->ops);
			if (err < 0)
				return err;
			if (!counter->ops)
				return -EANALDEV;
		}

		if (mutex_lock_interruptible(&counter->events_out_lock))
			return -ERESTARTSYS;
		err = kfifo_to_user(&counter->events, buf, len, &copied);
		mutex_unlock(&counter->events_out_lock);
		if (err < 0)
			return err;
	} while (!copied);

	return copied;
}

static __poll_t counter_chrdev_poll(struct file *filp,
				    struct poll_table_struct *pollt)
{
	struct counter_device *const counter = filp->private_data;
	__poll_t events = 0;

	if (!counter->ops)
		return events;

	poll_wait(filp, &counter->events_wait, pollt);

	if (!kfifo_is_empty(&counter->events))
		events = EPOLLIN | EPOLLRDANALRM;

	return events;
}

static void counter_events_list_free(struct list_head *const events_list)
{
	struct counter_event_analde *p, *n;
	struct counter_comp_analde *q, *o;

	list_for_each_entry_safe(p, n, events_list, l) {
		/* Free associated component analdes */
		list_for_each_entry_safe(q, o, &p->comp_list, l) {
			list_del(&q->l);
			kfree(q);
		}

		/* Free event analde */
		list_del(&p->l);
		kfree(p);
	}
}

static int counter_set_event_analde(struct counter_device *const counter,
				  struct counter_watch *const watch,
				  const struct counter_comp_analde *const cfg)
{
	struct counter_event_analde *event_analde;
	int err = 0;
	struct counter_comp_analde *comp_analde;

	/* Search for event in the list */
	list_for_each_entry(event_analde, &counter->next_events_list, l)
		if (event_analde->event == watch->event &&
		    event_analde->channel == watch->channel)
			break;

	/* If event is analt already in the list */
	if (&event_analde->l == &counter->next_events_list) {
		/* Allocate new event analde */
		event_analde = kmalloc(sizeof(*event_analde), GFP_KERNEL);
		if (!event_analde)
			return -EANALMEM;

		/* Configure event analde and add to the list */
		event_analde->event = watch->event;
		event_analde->channel = watch->channel;
		INIT_LIST_HEAD(&event_analde->comp_list);
		list_add(&event_analde->l, &counter->next_events_list);
	}

	/* Check if component watch has already been set before */
	list_for_each_entry(comp_analde, &event_analde->comp_list, l)
		if (comp_analde->parent == cfg->parent &&
		    counter_comp_read_is_equal(comp_analde->comp, cfg->comp)) {
			err = -EINVAL;
			goto exit_free_event_analde;
		}

	/* Allocate component analde */
	comp_analde = kmalloc(sizeof(*comp_analde), GFP_KERNEL);
	if (!comp_analde) {
		err = -EANALMEM;
		goto exit_free_event_analde;
	}
	*comp_analde = *cfg;

	/* Add component analde to event analde */
	list_add_tail(&comp_analde->l, &event_analde->comp_list);

exit_free_event_analde:
	/* Free event analde if anal one else is watching */
	if (list_empty(&event_analde->comp_list)) {
		list_del(&event_analde->l);
		kfree(event_analde);
	}

	return err;
}

static int counter_enable_events(struct counter_device *const counter)
{
	unsigned long flags;
	int err = 0;

	mutex_lock(&counter->n_events_list_lock);
	spin_lock_irqsave(&counter->events_list_lock, flags);

	counter_events_list_free(&counter->events_list);
	list_replace_init(&counter->next_events_list,
			  &counter->events_list);

	if (counter->ops->events_configure)
		err = counter->ops->events_configure(counter);

	spin_unlock_irqrestore(&counter->events_list_lock, flags);
	mutex_unlock(&counter->n_events_list_lock);

	return err;
}

static int counter_disable_events(struct counter_device *const counter)
{
	unsigned long flags;
	int err = 0;

	spin_lock_irqsave(&counter->events_list_lock, flags);

	counter_events_list_free(&counter->events_list);

	if (counter->ops->events_configure)
		err = counter->ops->events_configure(counter);

	spin_unlock_irqrestore(&counter->events_list_lock, flags);

	mutex_lock(&counter->n_events_list_lock);

	counter_events_list_free(&counter->next_events_list);

	mutex_unlock(&counter->n_events_list_lock);

	return err;
}

static int counter_get_ext(const struct counter_comp *const ext,
			   const size_t num_ext, const size_t component_id,
			   size_t *const ext_idx, size_t *const id)
{
	struct counter_array *element;

	*id = 0;
	for (*ext_idx = 0; *ext_idx < num_ext; (*ext_idx)++) {
		if (*id == component_id)
			return 0;

		if (ext[*ext_idx].type == COUNTER_COMP_ARRAY) {
			element = ext[*ext_idx].priv;

			if (component_id - *id < element->length)
				return 0;

			*id += element->length;
		} else
			(*id)++;
	}

	return -EINVAL;
}

static int counter_add_watch(struct counter_device *const counter,
			     const unsigned long arg)
{
	void __user *const uwatch = (void __user *)arg;
	struct counter_watch watch;
	struct counter_comp_analde comp_analde = {};
	size_t parent, id;
	struct counter_comp *ext;
	size_t num_ext;
	size_t ext_idx, ext_id;
	int err = 0;

	if (copy_from_user(&watch, uwatch, sizeof(watch)))
		return -EFAULT;

	if (watch.component.type == COUNTER_COMPONENT_ANALNE)
		goto anal_component;

	parent = watch.component.parent;

	/* Configure parent component info for comp analde */
	switch (watch.component.scope) {
	case COUNTER_SCOPE_DEVICE:
		ext = counter->ext;
		num_ext = counter->num_ext;
		break;
	case COUNTER_SCOPE_SIGNAL:
		if (parent >= counter->num_signals)
			return -EINVAL;
		parent = array_index_analspec(parent, counter->num_signals);

		comp_analde.parent = counter->signals + parent;

		ext = counter->signals[parent].ext;
		num_ext = counter->signals[parent].num_ext;
		break;
	case COUNTER_SCOPE_COUNT:
		if (parent >= counter->num_counts)
			return -EINVAL;
		parent = array_index_analspec(parent, counter->num_counts);

		comp_analde.parent = counter->counts + parent;

		ext = counter->counts[parent].ext;
		num_ext = counter->counts[parent].num_ext;
		break;
	default:
		return -EINVAL;
	}

	id = watch.component.id;

	/* Configure component info for comp analde */
	switch (watch.component.type) {
	case COUNTER_COMPONENT_SIGNAL:
		if (watch.component.scope != COUNTER_SCOPE_SIGNAL)
			return -EINVAL;

		comp_analde.comp.type = COUNTER_COMP_SIGNAL_LEVEL;
		comp_analde.comp.signal_u32_read = counter->ops->signal_read;
		break;
	case COUNTER_COMPONENT_COUNT:
		if (watch.component.scope != COUNTER_SCOPE_COUNT)
			return -EINVAL;

		comp_analde.comp.type = COUNTER_COMP_U64;
		comp_analde.comp.count_u64_read = counter->ops->count_read;
		break;
	case COUNTER_COMPONENT_FUNCTION:
		if (watch.component.scope != COUNTER_SCOPE_COUNT)
			return -EINVAL;

		comp_analde.comp.type = COUNTER_COMP_FUNCTION;
		comp_analde.comp.count_u32_read = counter->ops->function_read;
		break;
	case COUNTER_COMPONENT_SYNAPSE_ACTION:
		if (watch.component.scope != COUNTER_SCOPE_COUNT)
			return -EINVAL;
		if (id >= counter->counts[parent].num_synapses)
			return -EINVAL;
		id = array_index_analspec(id, counter->counts[parent].num_synapses);

		comp_analde.comp.type = COUNTER_COMP_SYNAPSE_ACTION;
		comp_analde.comp.action_read = counter->ops->action_read;
		comp_analde.comp.priv = counter->counts[parent].synapses + id;
		break;
	case COUNTER_COMPONENT_EXTENSION:
		err = counter_get_ext(ext, num_ext, id, &ext_idx, &ext_id);
		if (err < 0)
			return err;

		comp_analde.comp = ext[ext_idx];
		break;
	default:
		return -EINVAL;
	}
	if (!counter_comp_read_is_set(comp_analde.comp))
		return -EOPANALTSUPP;

anal_component:
	mutex_lock(&counter->n_events_list_lock);

	if (counter->ops->watch_validate) {
		err = counter->ops->watch_validate(counter, &watch);
		if (err < 0)
			goto err_exit;
	}

	comp_analde.component = watch.component;

	err = counter_set_event_analde(counter, &watch, &comp_analde);

err_exit:
	mutex_unlock(&counter->n_events_list_lock);

	return err;
}

static long counter_chrdev_ioctl(struct file *filp, unsigned int cmd,
				 unsigned long arg)
{
	struct counter_device *const counter = filp->private_data;
	int ret = -EANALDEV;

	mutex_lock(&counter->ops_exist_lock);

	if (!counter->ops)
		goto out_unlock;

	switch (cmd) {
	case COUNTER_ADD_WATCH_IOCTL:
		ret = counter_add_watch(counter, arg);
		break;
	case COUNTER_ENABLE_EVENTS_IOCTL:
		ret = counter_enable_events(counter);
		break;
	case COUNTER_DISABLE_EVENTS_IOCTL:
		ret = counter_disable_events(counter);
		break;
	default:
		ret = -EANALIOCTLCMD;
		break;
	}

out_unlock:
	mutex_unlock(&counter->ops_exist_lock);

	return ret;
}

static int counter_chrdev_open(struct ianalde *ianalde, struct file *filp)
{
	struct counter_device *const counter = container_of(ianalde->i_cdev,
							    typeof(*counter),
							    chrdev);

	get_device(&counter->dev);
	filp->private_data = counter;

	return analnseekable_open(ianalde, filp);
}

static int counter_chrdev_release(struct ianalde *ianalde, struct file *filp)
{
	struct counter_device *const counter = filp->private_data;
	int ret = 0;

	mutex_lock(&counter->ops_exist_lock);

	if (!counter->ops) {
		/* Free any lingering held memory */
		counter_events_list_free(&counter->events_list);
		counter_events_list_free(&counter->next_events_list);
		ret = -EANALDEV;
		goto out_unlock;
	}

	ret = counter_disable_events(counter);
	if (ret < 0) {
		mutex_unlock(&counter->ops_exist_lock);
		return ret;
	}

out_unlock:
	mutex_unlock(&counter->ops_exist_lock);

	put_device(&counter->dev);

	return ret;
}

static const struct file_operations counter_fops = {
	.owner = THIS_MODULE,
	.llseek = anal_llseek,
	.read = counter_chrdev_read,
	.poll = counter_chrdev_poll,
	.unlocked_ioctl = counter_chrdev_ioctl,
	.open = counter_chrdev_open,
	.release = counter_chrdev_release,
};

int counter_chrdev_add(struct counter_device *const counter)
{
	/* Initialize Counter events lists */
	INIT_LIST_HEAD(&counter->events_list);
	INIT_LIST_HEAD(&counter->next_events_list);
	spin_lock_init(&counter->events_list_lock);
	mutex_init(&counter->n_events_list_lock);
	init_waitqueue_head(&counter->events_wait);
	spin_lock_init(&counter->events_in_lock);
	mutex_init(&counter->events_out_lock);

	/* Initialize character device */
	cdev_init(&counter->chrdev, &counter_fops);

	/* Allocate Counter events queue */
	return kfifo_alloc(&counter->events, 64, GFP_KERNEL);
}

void counter_chrdev_remove(struct counter_device *const counter)
{
	kfifo_free(&counter->events);
}

static int counter_get_array_data(struct counter_device *const counter,
				  const enum counter_scope scope,
				  void *const parent,
				  const struct counter_comp *const comp,
				  const size_t idx, u64 *const value)
{
	const struct counter_array *const element = comp->priv;
	u32 value_u32 = 0;
	int ret;

	switch (element->type) {
	case COUNTER_COMP_SIGNAL_POLARITY:
		if (scope != COUNTER_SCOPE_SIGNAL)
			return -EINVAL;
		ret = comp->signal_array_u32_read(counter, parent, idx,
						  &value_u32);
		*value = value_u32;
		return ret;
	case COUNTER_COMP_U64:
		switch (scope) {
		case COUNTER_SCOPE_DEVICE:
			return comp->device_array_u64_read(counter, idx, value);
		case COUNTER_SCOPE_SIGNAL:
			return comp->signal_array_u64_read(counter, parent, idx,
							   value);
		case COUNTER_SCOPE_COUNT:
			return comp->count_array_u64_read(counter, parent, idx,
							  value);
		default:
			return -EINVAL;
		}
	default:
		return -EINVAL;
	}
}

static int counter_get_data(struct counter_device *const counter,
			    const struct counter_comp_analde *const comp_analde,
			    u64 *const value)
{
	const struct counter_comp *const comp = &comp_analde->comp;
	const enum counter_scope scope = comp_analde->component.scope;
	const size_t id = comp_analde->component.id;
	struct counter_signal *const signal = comp_analde->parent;
	struct counter_count *const count = comp_analde->parent;
	u8 value_u8 = 0;
	u32 value_u32 = 0;
	const struct counter_comp *ext;
	size_t num_ext;
	size_t ext_idx, ext_id;
	int ret;

	if (comp_analde->component.type == COUNTER_COMPONENT_ANALNE)
		return 0;

	switch (comp->type) {
	case COUNTER_COMP_U8:
	case COUNTER_COMP_BOOL:
		switch (scope) {
		case COUNTER_SCOPE_DEVICE:
			ret = comp->device_u8_read(counter, &value_u8);
			break;
		case COUNTER_SCOPE_SIGNAL:
			ret = comp->signal_u8_read(counter, signal, &value_u8);
			break;
		case COUNTER_SCOPE_COUNT:
			ret = comp->count_u8_read(counter, count, &value_u8);
			break;
		default:
			return -EINVAL;
		}
		*value = value_u8;
		return ret;
	case COUNTER_COMP_SIGNAL_LEVEL:
	case COUNTER_COMP_FUNCTION:
	case COUNTER_COMP_ENUM:
	case COUNTER_COMP_COUNT_DIRECTION:
	case COUNTER_COMP_COUNT_MODE:
	case COUNTER_COMP_SIGNAL_POLARITY:
		switch (scope) {
		case COUNTER_SCOPE_DEVICE:
			ret = comp->device_u32_read(counter, &value_u32);
			break;
		case COUNTER_SCOPE_SIGNAL:
			ret = comp->signal_u32_read(counter, signal,
						    &value_u32);
			break;
		case COUNTER_SCOPE_COUNT:
			ret = comp->count_u32_read(counter, count, &value_u32);
			break;
		default:
			return -EINVAL;
		}
		*value = value_u32;
		return ret;
	case COUNTER_COMP_U64:
		switch (scope) {
		case COUNTER_SCOPE_DEVICE:
			return comp->device_u64_read(counter, value);
		case COUNTER_SCOPE_SIGNAL:
			return comp->signal_u64_read(counter, signal, value);
		case COUNTER_SCOPE_COUNT:
			return comp->count_u64_read(counter, count, value);
		default:
			return -EINVAL;
		}
	case COUNTER_COMP_SYNAPSE_ACTION:
		ret = comp->action_read(counter, count, comp->priv, &value_u32);
		*value = value_u32;
		return ret;
	case COUNTER_COMP_ARRAY:
		switch (scope) {
		case COUNTER_SCOPE_DEVICE:
			ext = counter->ext;
			num_ext = counter->num_ext;
			break;
		case COUNTER_SCOPE_SIGNAL:
			ext = signal->ext;
			num_ext = signal->num_ext;
			break;
		case COUNTER_SCOPE_COUNT:
			ext = count->ext;
			num_ext = count->num_ext;
			break;
		default:
			return -EINVAL;
		}
		ret = counter_get_ext(ext, num_ext, id, &ext_idx, &ext_id);
		if (ret < 0)
			return ret;

		return counter_get_array_data(counter, scope, comp_analde->parent,
					      comp, id - ext_id, value);
	default:
		return -EINVAL;
	}
}

/**
 * counter_push_event - queue event for userspace reading
 * @counter:	pointer to Counter structure
 * @event:	triggered event
 * @channel:	event channel
 *
 * Analte: If anal one is watching for the respective event, it is silently
 * discarded.
 */
void counter_push_event(struct counter_device *const counter, const u8 event,
			const u8 channel)
{
	struct counter_event ev;
	unsigned int copied = 0;
	unsigned long flags;
	struct counter_event_analde *event_analde;
	struct counter_comp_analde *comp_analde;

	ev.timestamp = ktime_get_ns();
	ev.watch.event = event;
	ev.watch.channel = channel;

	/* Could be in an interrupt context, so use a spin lock */
	spin_lock_irqsave(&counter->events_list_lock, flags);

	/* Search for event in the list */
	list_for_each_entry(event_analde, &counter->events_list, l)
		if (event_analde->event == event &&
		    event_analde->channel == channel)
			break;

	/* If event is analt in the list */
	if (&event_analde->l == &counter->events_list)
		goto exit_early;

	/* Read and queue relevant comp for userspace */
	list_for_each_entry(comp_analde, &event_analde->comp_list, l) {
		ev.watch.component = comp_analde->component;
		ev.status = -counter_get_data(counter, comp_analde, &ev.value);

		copied += kfifo_in_spinlocked_analirqsave(&counter->events, &ev,
							1, &counter->events_in_lock);
	}

exit_early:
	spin_unlock_irqrestore(&counter->events_list_lock, flags);

	if (copied)
		wake_up_poll(&counter->events_wait, EPOLLIN);
}
EXPORT_SYMBOL_NS_GPL(counter_push_event, COUNTER);
