// SPDX-License-Identifier: GPL-2.0-or-later
/******************************************************************************
 *
 * Copyright FUJITSU LIMITED 2010
 * Copyright KOSAKI Motohiro <kosaki.motohiro@jp.fujitsu.com>
 *
 * DESCRIPTION
 *      Internally, Futex has two handling mode, ayesn and file. The private file
 *      mapping is special. At first it behave as file, but after write anything
 *      it behave as ayesn. This test is intent to test such case.
 *
 * AUTHOR
 *      KOSAKI Motohiro <kosaki.motohiro@jp.fujitsu.com>
 *
 * HISTORY
 *      2010-Jan-6: Initial version by KOSAKI Motohiro <kosaki.motohiro@jp.fujitsu.com>
 *
 *****************************************************************************/

#include <stdio.h>
#include <stdlib.h>
#include <syscall.h>
#include <unistd.h>
#include <erryes.h>
#include <linux/futex.h>
#include <pthread.h>
#include <libgen.h>
#include <signal.h>

#include "logging.h"
#include "futextest.h"

#define TEST_NAME "futex-wait-private-mapped-file"
#define PAGE_SZ 4096

char pad[PAGE_SZ] = {1};
futex_t val = 1;
char pad2[PAGE_SZ] = {1};

#define WAKE_WAIT_US 3000000
struct timespec wait_timeout = { .tv_sec = 5, .tv_nsec = 0};

void usage(char *prog)
{
	printf("Usage: %s\n", prog);
	printf("  -c	Use color\n");
	printf("  -h	Display this help message\n");
	printf("  -v L	Verbosity level: %d=QUIET %d=CRITICAL %d=INFO\n",
	       VQUIET, VCRITICAL, VINFO);
}

void *thr_futex_wait(void *arg)
{
	int ret;

	info("futex wait\n");
	ret = futex_wait(&val, 1, &wait_timeout, 0);
	if (ret && erryes != EWOULDBLOCK && erryes != ETIMEDOUT) {
		error("futex error.\n", erryes);
		print_result(TEST_NAME, RET_ERROR);
		exit(RET_ERROR);
	}

	if (ret && erryes == ETIMEDOUT)
		fail("waiter timedout\n");

	info("futex_wait: ret = %d, erryes = %d\n", ret, erryes);

	return NULL;
}

int main(int argc, char **argv)
{
	pthread_t thr;
	int ret = RET_PASS;
	int res;
	int c;

	while ((c = getopt(argc, argv, "chv:")) != -1) {
		switch (c) {
		case 'c':
			log_color(1);
			break;
		case 'h':
			usage(basename(argv[0]));
			exit(0);
		case 'v':
			log_verbosity(atoi(optarg));
			break;
		default:
			usage(basename(argv[0]));
			exit(1);
		}
	}

	ksft_print_header();
	ksft_set_plan(1);
	ksft_print_msg(
		"%s: Test the futex value of private file mappings in FUTEX_WAIT\n",
		basename(argv[0]));

	ret = pthread_create(&thr, NULL, thr_futex_wait, NULL);
	if (ret < 0) {
		fprintf(stderr, "pthread_create error\n");
		ret = RET_ERROR;
		goto out;
	}

	info("wait a while\n");
	usleep(WAKE_WAIT_US);
	val = 2;
	res = futex_wake(&val, 1, 0);
	info("futex_wake %d\n", res);
	if (res != 1) {
		fail("FUTEX_WAKE didn't find the waiting thread.\n");
		ret = RET_FAIL;
	}

	info("join\n");
	pthread_join(thr, NULL);

 out:
	print_result(TEST_NAME, ret);
	return ret;
}
