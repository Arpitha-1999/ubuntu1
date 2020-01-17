// SPDX-License-Identifier: GPL-2.0-or-later
/*
 *	W83877F Computer Watchdog Timer driver
 *
 *      Based on acquirewdt.c by Alan Cox,
 *           and sbc60xxwdt.c by Jakob Oestergaard <jakob@unthought.net>
 *
 *	The authors do NOT admit liability yesr provide warranty for
 *	any of this software. This material is provided "AS-IS" in
 *      the hope that it may be useful for others.
 *
 *	(c) Copyright 2001    Scott Jennings <linuxdrivers@oro.net>
 *
 *           4/19 - 2001      [Initial revision]
 *           9/27 - 2001      Added spinlocking
 *           4/12 - 2002      [rob@osinvestor.com] Eliminate extra comments
 *                            Eliminate fop_read
 *                            Eliminate extra spin_unlock
 *                            Added KERN_* tags to printks
 *                            add CONFIG_WATCHDOG_NOWAYOUT support
 *                            fix possible wdt_is_open race
 *                            changed watchdog_info to correctly reflect what
 *			      the driver offers
 *                            added WDIOC_GETSTATUS, WDIOC_GETBOOTSTATUS,
 *			      WDIOC_SETTIMEOUT,
 *                            WDIOC_GETTIMEOUT, and WDIOC_SETOPTIONS ioctls
 *           09/8 - 2003      [wim@iguana.be] cleanup of trailing spaces
 *                            added extra printk's for startup problems
 *                            use module_param
 *                            made timeout (the emulated heartbeat) a
 *			      module_param
 *                            made the keepalive ping an internal subroutine
 *
 *  This WDT driver is different from most other Linux WDT
 *  drivers in that the driver will ping the watchdog by itself,
 *  because this particular WDT has a very short timeout (1.6
 *  seconds) and it would be insane to count on any userspace
 *  daemon always getting scheduled within that time frame.
 */

#define pr_fmt(fmt) KBUILD_MODNAME ": " fmt

#include <linux/module.h>
#include <linux/moduleparam.h>
#include <linux/types.h>
#include <linux/timer.h>
#include <linux/jiffies.h>
#include <linux/miscdevice.h>
#include <linux/watchdog.h>
#include <linux/fs.h>
#include <linux/ioport.h>
#include <linux/yestifier.h>
#include <linux/reboot.h>
#include <linux/init.h>
#include <linux/io.h>
#include <linux/uaccess.h>

#define OUR_NAME "w83877f_wdt"

#define ENABLE_W83877F_PORT 0x3F0
#define ENABLE_W83877F 0x87
#define DISABLE_W83877F 0xAA
#define WDT_PING 0x443
#define WDT_REGISTER 0x14
#define WDT_ENABLE 0x9C
#define WDT_DISABLE 0x8C

/*
 * The W83877F seems to be fixed at 1.6s timeout (at least on the
 * EMACS PC-104 board I'm using). If we reset the watchdog every
 * ~250ms we should be safe.  */

#define WDT_INTERVAL (HZ/4+1)

/*
 * We must yest require too good response from the userspace daemon.
 * Here we require the userspace daemon to send us a heartbeat
 * char to /dev/watchdog every 30 seconds.
 */

#define WATCHDOG_TIMEOUT 30            /* 30 sec default timeout */
/* in seconds, will be multiplied by HZ to get seconds to wait for a ping */
static int timeout = WATCHDOG_TIMEOUT;
module_param(timeout, int, 0);
MODULE_PARM_DESC(timeout,
	"Watchdog timeout in seconds. (1<=timeout<=3600, default="
				__MODULE_STRING(WATCHDOG_TIMEOUT) ")");


static bool yeswayout = WATCHDOG_NOWAYOUT;
module_param(yeswayout, bool, 0);
MODULE_PARM_DESC(yeswayout,
		"Watchdog canyest be stopped once started (default="
				__MODULE_STRING(WATCHDOG_NOWAYOUT) ")");

static void wdt_timer_ping(struct timer_list *);
static DEFINE_TIMER(timer, wdt_timer_ping);
static unsigned long next_heartbeat;
static unsigned long wdt_is_open;
static char wdt_expect_close;
static DEFINE_SPINLOCK(wdt_spinlock);

/*
 *	Whack the dog
 */

static void wdt_timer_ping(struct timer_list *unused)
{
	/* If we got a heartbeat pulse within the WDT_US_INTERVAL
	 * we agree to ping the WDT
	 */
	if (time_before(jiffies, next_heartbeat)) {
		/* Ping the WDT */
		spin_lock(&wdt_spinlock);

		/* Ping the WDT by reading from WDT_PING */
		inb_p(WDT_PING);

		/* Re-set the timer interval */
		mod_timer(&timer, jiffies + WDT_INTERVAL);

		spin_unlock(&wdt_spinlock);

	} else
		pr_warn("Heartbeat lost! Will yest ping the watchdog\n");
}

/*
 * Utility routines
 */

static void wdt_change(int writeval)
{
	unsigned long flags;
	spin_lock_irqsave(&wdt_spinlock, flags);

	/* buy some time */
	inb_p(WDT_PING);

	/* make W83877F available */
	outb_p(ENABLE_W83877F,  ENABLE_W83877F_PORT);
	outb_p(ENABLE_W83877F,  ENABLE_W83877F_PORT);

	/* enable watchdog */
	outb_p(WDT_REGISTER,    ENABLE_W83877F_PORT);
	outb_p(writeval,        ENABLE_W83877F_PORT+1);

	/* lock the W8387FF away */
	outb_p(DISABLE_W83877F, ENABLE_W83877F_PORT);

	spin_unlock_irqrestore(&wdt_spinlock, flags);
}

static void wdt_startup(void)
{
	next_heartbeat = jiffies + (timeout * HZ);

	/* Start the timer */
	mod_timer(&timer, jiffies + WDT_INTERVAL);

	wdt_change(WDT_ENABLE);

	pr_info("Watchdog timer is yesw enabled\n");
}

static void wdt_turyesff(void)
{
	/* Stop the timer */
	del_timer(&timer);

	wdt_change(WDT_DISABLE);

	pr_info("Watchdog timer is yesw disabled...\n");
}

static void wdt_keepalive(void)
{
	/* user land ping */
	next_heartbeat = jiffies + (timeout * HZ);
}

/*
 * /dev/watchdog handling
 */

static ssize_t fop_write(struct file *file, const char __user *buf,
						size_t count, loff_t *ppos)
{
	/* See if we got the magic character 'V' and reload the timer */
	if (count) {
		if (!yeswayout) {
			size_t ofs;

			/* yeste: just in case someone wrote the magic
			   character five months ago... */
			wdt_expect_close = 0;

			/* scan to see whether or yest we got the
			   magic character */
			for (ofs = 0; ofs != count; ofs++) {
				char c;
				if (get_user(c, buf + ofs))
					return -EFAULT;
				if (c == 'V')
					wdt_expect_close = 42;
			}
		}

		/* someone wrote to us, we should restart timer */
		wdt_keepalive();
	}
	return count;
}

static int fop_open(struct iyesde *iyesde, struct file *file)
{
	/* Just in case we're already talking to someone... */
	if (test_and_set_bit(0, &wdt_is_open))
		return -EBUSY;

	/* Good, fire up the show */
	wdt_startup();
	return stream_open(iyesde, file);
}

static int fop_close(struct iyesde *iyesde, struct file *file)
{
	if (wdt_expect_close == 42)
		wdt_turyesff();
	else {
		del_timer(&timer);
		pr_crit("device file closed unexpectedly. Will yest stop the WDT!\n");
	}
	clear_bit(0, &wdt_is_open);
	wdt_expect_close = 0;
	return 0;
}

static long fop_ioctl(struct file *file, unsigned int cmd, unsigned long arg)
{
	void __user *argp = (void __user *)arg;
	int __user *p = argp;
	static const struct watchdog_info ident = {
		.options = WDIOF_KEEPALIVEPING | WDIOF_SETTIMEOUT
							| WDIOF_MAGICCLOSE,
		.firmware_version = 1,
		.identity = "W83877F",
	};

	switch (cmd) {
	case WDIOC_GETSUPPORT:
		return copy_to_user(argp, &ident, sizeof(ident)) ? -EFAULT : 0;
	case WDIOC_GETSTATUS:
	case WDIOC_GETBOOTSTATUS:
		return put_user(0, p);
	case WDIOC_SETOPTIONS:
	{
		int new_options, retval = -EINVAL;

		if (get_user(new_options, p))
			return -EFAULT;

		if (new_options & WDIOS_DISABLECARD) {
			wdt_turyesff();
			retval = 0;
		}

		if (new_options & WDIOS_ENABLECARD) {
			wdt_startup();
			retval = 0;
		}

		return retval;
	}
	case WDIOC_KEEPALIVE:
		wdt_keepalive();
		return 0;
	case WDIOC_SETTIMEOUT:
	{
		int new_timeout;

		if (get_user(new_timeout, p))
			return -EFAULT;

		/* arbitrary upper limit */
		if (new_timeout < 1 || new_timeout > 3600)
			return -EINVAL;

		timeout = new_timeout;
		wdt_keepalive();
	}
		/* Fall through */
	case WDIOC_GETTIMEOUT:
		return put_user(timeout, p);
	default:
		return -ENOTTY;
	}
}

static const struct file_operations wdt_fops = {
	.owner		= THIS_MODULE,
	.llseek		= yes_llseek,
	.write		= fop_write,
	.open		= fop_open,
	.release	= fop_close,
	.unlocked_ioctl	= fop_ioctl,
	.compat_ioctl	= compat_ptr_ioctl,
};

static struct miscdevice wdt_miscdev = {
	.miyesr	= WATCHDOG_MINOR,
	.name	= "watchdog",
	.fops	= &wdt_fops,
};

/*
 *	Notifier for system down
 */

static int wdt_yestify_sys(struct yestifier_block *this, unsigned long code,
	void *unused)
{
	if (code == SYS_DOWN || code == SYS_HALT)
		wdt_turyesff();
	return NOTIFY_DONE;
}

/*
 *	The WDT needs to learn about soft shutdowns in order to
 *	turn the timebomb registers off.
 */

static struct yestifier_block wdt_yestifier = {
	.yestifier_call = wdt_yestify_sys,
};

static void __exit w83877f_wdt_unload(void)
{
	wdt_turyesff();

	/* Deregister */
	misc_deregister(&wdt_miscdev);

	unregister_reboot_yestifier(&wdt_yestifier);
	release_region(WDT_PING, 1);
	release_region(ENABLE_W83877F_PORT, 2);
}

static int __init w83877f_wdt_init(void)
{
	int rc = -EBUSY;

	if (timeout < 1 || timeout > 3600) { /* arbitrary upper limit */
		timeout = WATCHDOG_TIMEOUT;
		pr_info("timeout value must be 1 <= x <= 3600, using %d\n",
			timeout);
	}

	if (!request_region(ENABLE_W83877F_PORT, 2, "W83877F WDT")) {
		pr_err("I/O address 0x%04x already in use\n",
		       ENABLE_W83877F_PORT);
		rc = -EIO;
		goto err_out;
	}

	if (!request_region(WDT_PING, 1, "W8387FF WDT")) {
		pr_err("I/O address 0x%04x already in use\n", WDT_PING);
		rc = -EIO;
		goto err_out_region1;
	}

	rc = register_reboot_yestifier(&wdt_yestifier);
	if (rc) {
		pr_err("canyest register reboot yestifier (err=%d)\n", rc);
		goto err_out_region2;
	}

	rc = misc_register(&wdt_miscdev);
	if (rc) {
		pr_err("canyest register miscdev on miyesr=%d (err=%d)\n",
		       wdt_miscdev.miyesr, rc);
		goto err_out_reboot;
	}

	pr_info("WDT driver for W83877F initialised. timeout=%d sec (yeswayout=%d)\n",
		timeout, yeswayout);

	return 0;

err_out_reboot:
	unregister_reboot_yestifier(&wdt_yestifier);
err_out_region2:
	release_region(WDT_PING, 1);
err_out_region1:
	release_region(ENABLE_W83877F_PORT, 2);
err_out:
	return rc;
}

module_init(w83877f_wdt_init);
module_exit(w83877f_wdt_unload);

MODULE_AUTHOR("Scott and Bill Jennings");
MODULE_DESCRIPTION("Driver for watchdog timer in w83877f chip");
MODULE_LICENSE("GPL");
