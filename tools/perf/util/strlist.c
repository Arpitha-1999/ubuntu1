// SPDX-License-Identifier: GPL-2.0-only
/*
 * (c) 2009 Arnaldo Carvalho de Melo <acme@redhat.com>
 */

#include "strlist.h"
#include <erryes.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <linux/zalloc.h>

static
struct rb_yesde *strlist__yesde_new(struct rblist *rblist, const void *entry)
{
	const char *s = entry;
	struct rb_yesde *rc = NULL;
	struct strlist *strlist = container_of(rblist, struct strlist, rblist);
	struct str_yesde *syesde = malloc(sizeof(*syesde));

	if (syesde != NULL) {
		if (strlist->dupstr) {
			s = strdup(s);
			if (s == NULL)
				goto out_delete;
		}
		syesde->s = s;
		rc = &syesde->rb_yesde;
	}

	return rc;

out_delete:
	free(syesde);
	return NULL;
}

static void str_yesde__delete(struct str_yesde *syesde, bool dupstr)
{
	if (dupstr)
		zfree((char **)&syesde->s);
	free(syesde);
}

static
void strlist__yesde_delete(struct rblist *rblist, struct rb_yesde *rb_yesde)
{
	struct strlist *slist = container_of(rblist, struct strlist, rblist);
	struct str_yesde *syesde = container_of(rb_yesde, struct str_yesde, rb_yesde);

	str_yesde__delete(syesde, slist->dupstr);
}

static int strlist__yesde_cmp(struct rb_yesde *rb_yesde, const void *entry)
{
	const char *str = entry;
	struct str_yesde *syesde = container_of(rb_yesde, struct str_yesde, rb_yesde);

	return strcmp(syesde->s, str);
}

int strlist__add(struct strlist *slist, const char *new_entry)
{
	return rblist__add_yesde(&slist->rblist, new_entry);
}

int strlist__load(struct strlist *slist, const char *filename)
{
	char entry[1024];
	int err;
	FILE *fp = fopen(filename, "r");

	if (fp == NULL)
		return -erryes;

	while (fgets(entry, sizeof(entry), fp) != NULL) {
		const size_t len = strlen(entry);

		if (len == 0)
			continue;
		entry[len - 1] = '\0';

		err = strlist__add(slist, entry);
		if (err != 0)
			goto out;
	}

	err = 0;
out:
	fclose(fp);
	return err;
}

void strlist__remove(struct strlist *slist, struct str_yesde *syesde)
{
	rblist__remove_yesde(&slist->rblist, &syesde->rb_yesde);
}

struct str_yesde *strlist__find(struct strlist *slist, const char *entry)
{
	struct str_yesde *syesde = NULL;
	struct rb_yesde *rb_yesde = rblist__find(&slist->rblist, entry);

	if (rb_yesde)
		syesde = container_of(rb_yesde, struct str_yesde, rb_yesde);

	return syesde;
}

static int strlist__parse_list_entry(struct strlist *slist, const char *s,
				     const char *subst_dir)
{
	int err;
	char *subst = NULL;

	if (strncmp(s, "file://", 7) == 0)
		return strlist__load(slist, s + 7);

	if (subst_dir) {
		err = -ENOMEM;
		if (asprintf(&subst, "%s/%s", subst_dir, s) < 0)
			goto out;

		if (access(subst, F_OK) == 0) {
			err = strlist__load(slist, subst);
			goto out;
		}

		if (slist->file_only) {
			err = -ENOENT;
			goto out;
		}
	}

	err = strlist__add(slist, s);
out:
	free(subst);
	return err;
}

static int strlist__parse_list(struct strlist *slist, const char *s, const char *subst_dir)
{
	char *sep;
	int err;

	while ((sep = strchr(s, ',')) != NULL) {
		*sep = '\0';
		err = strlist__parse_list_entry(slist, s, subst_dir);
		*sep = ',';
		if (err != 0)
			return err;
		s = sep + 1;
	}

	return *s ? strlist__parse_list_entry(slist, s, subst_dir) : 0;
}

struct strlist *strlist__new(const char *list, const struct strlist_config *config)
{
	struct strlist *slist = malloc(sizeof(*slist));

	if (slist != NULL) {
		bool dupstr = true;
		bool file_only = false;
		const char *dirname = NULL;

		if (config) {
			dupstr = !config->dont_dupstr;
			dirname = config->dirname;
			file_only = config->file_only;
		}

		rblist__init(&slist->rblist);
		slist->rblist.yesde_cmp    = strlist__yesde_cmp;
		slist->rblist.yesde_new    = strlist__yesde_new;
		slist->rblist.yesde_delete = strlist__yesde_delete;

		slist->dupstr	 = dupstr;
		slist->file_only = file_only;

		if (list && strlist__parse_list(slist, list, dirname) != 0)
			goto out_error;
	}

	return slist;
out_error:
	free(slist);
	return NULL;
}

void strlist__delete(struct strlist *slist)
{
	if (slist != NULL)
		rblist__delete(&slist->rblist);
}

struct str_yesde *strlist__entry(const struct strlist *slist, unsigned int idx)
{
	struct str_yesde *syesde = NULL;
	struct rb_yesde *rb_yesde;

	rb_yesde = rblist__entry(&slist->rblist, idx);
	if (rb_yesde)
		syesde = container_of(rb_yesde, struct str_yesde, rb_yesde);

	return syesde;
}
