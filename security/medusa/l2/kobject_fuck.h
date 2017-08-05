/* kobject_fuck.c, (C) 2002 Milan Pikula */

#include <linux/module.h>
#include <linux/init.h>
#include <linux/namei.h>
#include <linux/fs.h>
#include <linux/path.h>
#include <linux/kernel.h>
#include <linux/errno.h>
#include <linux/medusa/l3/registry.h>
#include <linux/medusa/l1/inode.h>

#define inode_security(inode) (*(struct medusa_l1_inode_s*)(inode->i_security))

//int validate_fuck_link(struct dentry *old_dentry, const struct path *fuck_path, struct dentry *new_dentry);
int validate_fuck_link(struct dentry *old_dentry);
int validate_fuck(struct path fuck_path);
int fuck_free(struct medusa_l1_inode_s* med);


struct fuck_kobject {	
	MEDUSA_KOBJECT_HEADER;
	char path[PATH_MAX];
	unsigned long i_ino;
	char action[20];
	MEDUSA_OBJECT_VARS;
};
extern MED_DECLARE_KCLASSOF(fuck_kobject);

