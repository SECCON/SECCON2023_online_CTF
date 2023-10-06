#include <linux/module.h>
#include "kmemo/char.h"
#include "kmemo/pci.h"

MODULE_LICENSE("GPL v2");
MODULE_AUTHOR("ShiftCrops");
MODULE_DESCRIPTION("SECCON CTF 2023 Online Challenge : Temporary Memo Module");

static int __init module_initialize(void){
	int ret;

	if((ret = reg_chrdev()) < 0)
		goto ERR;

	if((ret = reg_pcidrv()) < 0)
		unreg_chrdev();

ERR:
	return ret;
}

static void __exit module_finalize(void){
	unreg_pcidrv();
	unreg_chrdev();
}

module_init(module_initialize);
module_exit(module_finalize);

