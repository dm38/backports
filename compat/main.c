#include <linux/module.h>
#include <linux/init.h>
#include <linux/pm_qos.h>
#include <linux/workqueue.h>
#include "backports.h"

MODULE_AUTHOR("Luis R. Rodriguez");
MODULE_AUTHOR("Igor Mokrushin");
MODULE_DESCRIPTION("Kernel backport module");
MODULE_LICENSE("GPL");

#ifndef CPTCFG_KERNEL_NAME
#error "You need a CPTCFG_KERNEL_NAME"
#endif

#ifndef CPTCFG_KERNEL_VERSION
#error "You need a CPTCFG_KERNEL_VERSION"
#endif

#ifndef CPTCFG_VERSION
#error "You need a CPTCFG_VERSION"
#endif

static char *backported_kernel_name = CPTCFG_KERNEL_NAME;

module_param(backported_kernel_name, charp, 0400);
MODULE_PARM_DESC(backported_kernel_name,
		 "The kernel tree name that was used for this backport (" CPTCFG_KERNEL_NAME ")");

#ifdef BACKPORTS_GIT_TRACKED
static char *backports_tracker_id = BACKPORTS_GIT_TRACKED;
module_param(backports_tracker_id, charp, 0400);
MODULE_PARM_DESC(backports_tracker_id,
		 "The version of the tree containing this backport (" BACKPORTS_GIT_TRACKED ")");
#else
static char *backported_kernel_version = CPTCFG_KERNEL_VERSION;
static char *backports_version = CPTCFG_VERSION;

module_param(backported_kernel_version, charp, 0400);
MODULE_PARM_DESC(backported_kernel_version,
		 "The kernel version that was used for this backport (" CPTCFG_KERNEL_VERSION ")");

module_param(backports_version, charp, 0400);
MODULE_PARM_DESC(backports_version,
		 "The git version of the backports tree used to generate this backport (" CPTCFG_VERSION ")");

#endif

void backport_dependency_symbol(void)
{
}
EXPORT_SYMBOL_GPL(backport_dependency_symbol);

extern int clk_disable_unused(void);
#ifndef CPTCFG_REGMAP_I2C
extern int i2c_init(void);
extern void i2c_exit(void);
extern int i2c_dev_init(void);
extern void i2c_dev_exit(void);
#else
static int i2c_init(void) { return 0; }
static void i2c_exit(void) {}
static int i2c_dev_init(void) { return 0; }
static void i2c_dev_exit(void) {}
#endif
#ifdef CPTCFG_FW_LOADER_USER_HELPER
extern int firmware_class_init(void);
extern void firmware_class_exit(void);
#else
static int firmware_class_init(void) { return 0; }
static void firmware_class_exit(void) {}
#endif

static int __init backport_init(void)
{
	int ret = crypto_ccm_module_init();
	if (ret)
		return ret;

	ret = devcoredump_init();
	if (ret) {
		crypto_ccm_module_exit();
		return ret;
	}

	clk_disable_unused();

	ret = firmware_class_init();
	if (ret) {
		crypto_ccm_module_exit();
		devcoredump_exit();
		return ret;
	}

	ret = i2c_init();
	if (ret) {
		crypto_ccm_module_exit();
		devcoredump_exit();
		firmware_class_exit();
		return ret;
	}

	ret = i2c_dev_init();
	if (ret) {
		i2c_exit();
		crypto_ccm_module_exit();
		devcoredump_exit();
		firmware_class_exit();
		return ret;
	}
	printk(KERN_INFO "Loading modules backported from " CPTCFG_KERNEL_NAME
#ifndef BACKPORTS_GIT_TRACKED
		" version " CPTCFG_KERNEL_VERSION
#endif
		"\n");
#ifdef BACKPORTS_GIT_TRACKED
	printk(KERN_INFO BACKPORTS_GIT_TRACKED "\n");
#else

#ifdef CONFIG_BACKPORT_INTEGRATE
	printk(KERN_INFO "Backport integrated by backports.git " CPTCFG_VERSION "\n");
#else
	printk(KERN_INFO "Backport generated by backports.git " CPTCFG_VERSION "\n");
#endif /* CONFIG_BACKPORT_INTEGRATE */

#endif /* BACKPORTS_GIT_TRACKED */

        return 0;
}
subsys_initcall(backport_init);

static void __exit backport_exit(void)
{
	crypto_ccm_module_exit();
	devcoredump_exit();
	i2c_dev_exit();
	i2c_exit();
	firmware_class_exit();
}
module_exit(backport_exit);
