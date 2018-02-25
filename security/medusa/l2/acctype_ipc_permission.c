#include <linux/medusa/l3/registry.h>
#include <linux/medusa/l1/task.h>
#include <linux/medusa/l1/ipc.h>
#include <linux/init.h>
#include <linux/mm.h>
#include "kobject_process.h"
#include "kobject_ipc_common.h"
#include "evtype_ipc.h"

/* let's define the 'fork' access type, with object=task and subject=task. */

struct ipc_perm_access {
	MEDUSA_ACCESS_HEADER;
	unsigned int ipc_class;
	u32 perms;
};

MED_ATTRS(ipc_perm_access) {
	MED_ATTR_RO (ipc_perm_access, perms, "perms", MED_UNSIGNED),
	MED_ATTR_RO (ipc_perm_access, ipc_class, "ipc_class", MED_UNSIGNED),
	MED_ATTR_END
};

MED_ACCTYPE(ipc_perm_access, "ipc_perm", process_kobject, "process", ipc_kobject, "object");

int __init ipc_acctype_init(void) {
	MED_REGISTER_ACCTYPE(ipc_perm_access,MEDUSA_ACCTYPE_TRIGGEREDATSUBJECT);
	return 0;
}

medusa_answer_t medusa_ipc_permission(struct kern_ipc_perm *ipcp, u32 perms)
{
	medusa_answer_t retval = MED_OK;
	struct ipc_perm_access access;
	struct process_kobject process;
	struct ipc_kobject object;
	
    memset(&access, '\0', sizeof(struct ipc_perm_access));
    /* process_kobject parent is zeroed by process_kern2kobj function */

	if (!MED_MAGIC_VALID(&task_security(current)) && process_kobj_validate_task(current) <= 0)
		return MED_OK;
	if (!MED_MAGIC_VALID(ipc_security(ipcp)) && medusa_ipc_validate(ipcp) <= 0)
		return MED_OK;

	if (MEDUSA_MONITORED_ACCESS_S(ipc_perm_access, &task_security(current))) {
		access.perms = ipc_security(ipcp)->ipc_class;
		process_kern2kobj(&process, current);
		if(ipc_kern2kobj(&object, ipcp) == 0) {
			retval = MED_DECIDE(ipc_perm_access, &access, &process, &object);
			if (retval == MED_ERR)
				retval = MED_OK;
		}
	}
	return retval;
}
__initcall(ipc_acctype_init);
