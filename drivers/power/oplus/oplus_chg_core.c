#define pr_fmt(fmt) "OPLUS_CHG[CORE]: %s[%d]: " fmt, __func__, __LINE__

#include <linux/module.h>
#include <linux/types.h>
#include <linux/init.h>
#include <linux/slab.h>
#include <linux/delay.h>
#include <linux/device.h>
#include <linux/notifier.h>
#include <linux/err.h>
#include <linux/of.h>
#include <linux/power_supply.h>
#include <linux/property.h>
#include "oplus_chg_module.h"
#include "oplus_chg_core.h"

#ifdef MODULE
__attribute__((weak)) size_t __oplus_chg_module_start;
__attribute__((weak)) size_t __oplus_chg_module_end;

static inline struct oplus_chg_module *oplus_chg_find_first_module(void)
{
	return (struct oplus_chg_module *)&__oplus_chg_module_start;
}

static inline struct oplus_chg_module *oplus_chg_find_last_module(void)
{
	struct oplus_chg_module *oplus_module = (struct oplus_chg_module *)&__oplus_chg_module_end;
	return oplus_module--;
}
#endif /* MODULE */

static int __init oplus_chg_class_init(void)
{
	int rc;
#ifdef MODULE
	struct oplus_chg_module *oplus_module;
	struct oplus_chg_module *last_oplus_module;
#endif

#ifdef MODULE
	oplus_module = oplus_chg_find_first_module();
	last_oplus_module = oplus_chg_find_last_module();
	while ((size_t)oplus_module <= (size_t)last_oplus_module) {
		if ((oplus_module->magic == OPLUS_CHG_MODEL_MAGIC) &&
		    (oplus_module->chg_module_init != NULL)) {
			rc = oplus_module->chg_module_init();
			if (rc < 0) {
				pr_err("%s init error, rc=%d\n", oplus_module->name, rc);
				oplus_module--;
				goto module_init_err;
			}
		}
		oplus_module++;
	}
#endif /* MODULE */

	return 0;

#ifdef MODULE
module_init_err:
	while ((size_t)oplus_module >= (size_t)&__oplus_chg_module_start) {
		if ((oplus_module->magic == OPLUS_CHG_MODEL_MAGIC) &&
		    (oplus_module->chg_module_exit != NULL))
			oplus_module->chg_module_exit();
		oplus_module--;
	}
#endif /* MODULE */
	return rc;
}

static void __exit oplus_chg_class_exit(void)
{
#ifdef MODULE
	struct oplus_chg_module *oplus_module;

	oplus_module = oplus_chg_find_last_module();
	while ((size_t)oplus_module >= (size_t)&__oplus_chg_module_start) {
		if ((oplus_module->magic == OPLUS_CHG_MODEL_MAGIC) &&
		    (oplus_module->chg_module_exit != NULL))
			oplus_module->chg_module_exit();
		oplus_module--;
	}
#endif /* MODULE */
}

subsys_initcall(oplus_chg_class_init);
module_exit(oplus_chg_class_exit);

MODULE_DESCRIPTION("oplus charge management subsystem");
MODULE_AUTHOR("Nick Hu <nick.hu@oneplus.com>");
MODULE_LICENSE("GPL");
