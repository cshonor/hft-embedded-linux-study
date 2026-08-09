#include <linux/init.h>
#include <linux/kernel.h>
#include <linux/module.h>

MODULE_LICENSE("GPL");
MODULE_DESCRIPTION("P4 Part B /proc stub");

static int __init proc_stub_init(void)
{
	pr_info("p4_proc_stub: loaded (add /proc next)\n");
	return 0;
}

static void __exit proc_stub_exit(void)
{
	pr_info("p4_proc_stub: unloaded\n");
}

module_init(proc_stub_init);
module_exit(proc_stub_exit);
