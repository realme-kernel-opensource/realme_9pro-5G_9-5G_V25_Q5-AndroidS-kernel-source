/*
* Copyright (c), 2008-2019 , Guangdong OPLUS Mobile Comm Corp., Ltd.
* File: ufs-oplus.c
* Description: UFS GKI
* Version: 1.0
* Date: 2020-08-12
* ------------------------------ Revision History: --------------------------------
* <version>           <date>                <author>                            <desc>
***********************************************************************************/

#include <linux/module.h>
#include <trace/hooks/oplus_ufs.h>
#include <linux/delay.h>
#include <soc/oplus/device_info.h>
#include <linux/proc_fs.h>

static bool flag_retry = 0;
struct proc_dir_entry *ufs_proc_dir;

void ufs_extra_query_retry_handle(void *data, int err, bool flag_res, int *i)
{
	if (!err && flag_res && (*i == 999) && !flag_retry) {
		flag_retry = 1;
		*i = 499;
	}

	return;
}

void ufs_gen_proc_devinfo_handle(void *data, struct ufs_hba *hba)
{
	int ret = 0;
	static char temp_version[5] = {0};
	static char vendor[9] = {0};
	static char model[17] = {0};

	strncpy(temp_version, hba->sdev_ufs_device->rev, 4);
	strncpy(vendor, hba->sdev_ufs_device->vendor, 8);
	strncpy(model, hba->sdev_ufs_device->model, 16);
	ret = register_device_proc("ufs_version", temp_version, vendor);
	if (ret) {
		printk("Fail register_device_proc ufs_version\n");
	}
	ret = register_device_proc("ufs", model, vendor);
	if (ret) {
		printk("Fail register_device_proc ufs \n");
	}

	return;
}

static int __init
oplus_ufs_common_init(void)
{
	int rc;

	printk("oplus_ufs_common_init");

	rc = register_trace_android_vh_ufs_extra_query_retry(ufs_extra_query_retry_handle, NULL);
	if (rc != 0)
		pr_err("register_trace_android_vh_ufs_extra_query_retry failed! rc=%d\n", rc);

	rc = register_trace_android_vh_ufs_gen_proc_devinfo(ufs_gen_proc_devinfo_handle, NULL);
	if (rc != 0)
		pr_err("register_trace_android_vh_ufs_gen_proc_devinfo failed! rc=%d\n", rc);

	return rc;
}

device_initcall(oplus_ufs_common_init);

MODULE_DESCRIPTION("OPLUS ufs driver common");
MODULE_LICENSE("GPL v2");
MODULE_AUTHOR("Tianwen");
