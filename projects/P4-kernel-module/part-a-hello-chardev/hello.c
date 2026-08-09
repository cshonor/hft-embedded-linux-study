#include <linux/init.h>
#include <linux/kernel.h>
#include <linux/module.h>

MODULE_LICENSE("GPL");
MODULE_AUTHOR("hft learner");
MODULE_DESCRIPTION("P4 Part A hello stub");

static int __init hello_init(void)
{
	pr_info("p4_hello: loaded\n");
	return 0;
}

static void __exit hello_exit(void)
{
	pr_info("p4_hello: unloaded\n");
}

module_init(hello_init);
module_exit(hello_exit);
