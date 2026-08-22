/* 第一个 Linux 内核模块（LKM）：Hello World
 *
 * 和用户态 hello.c 的根本区别：
 *   用户态  →  gcc hello.c -o hello  →  ./hello        （ELF 可执行，execve 起进程）
 *   内核态  →  make                  →  insmod hello.ko（ELF 可重定位，链接进内核）
 *
 * 不能直接 ./hello.ko 跑！它没有 main()，没有进程上下文；init 函数在 insmod
 * 时被内核调用一次，返回后模块驻留在内核地址空间，直到 rmmod 调 exit。
 *
 * 编译/加载/卸载流程见同目录 Makefile 头注释。
 */
#include <linux/init.h>    /* module_init / __init / __exit        */
#include <linux/module.h> /* MODULE_LICENSE / MODULE_AUTHOR 等宏  */
#include <linux/kernel.h> /* printk, KERN_INFO 等日志等级宏       */

/* 模块元信息，modinfo hello.ko 能读出来；不写 LICENSE 会 taint 内核 */
MODULE_LICENSE("GPL");
MODULE_AUTHOR("Your Name");
MODULE_DESCRIPTION("A simple hello world kernel module");
MODULE_VERSION("0.01");

/* __init = __section(.init.text)：加载完执行一次后，该页可释放回伙伴系统
 * 返回 0 = 加载成功；返回非 0 = 加载失败，内核自动回滚已注册的资源
 */
static int __init hello_init(void)
{
	/* printk 不是 printf：内核没有 stdio，输出写进内核环形缓冲区，
	 * 用 dmesg 读。KERN_INFO 是日志等级（<6>），低于 console_loglevel
	 * 才会同时打到控制台。
	 */
	printk(KERN_INFO "Hello, World! I am a kernel module.\n");
	return 0;
}

/* __exit = __section(.exit.text)：编进内核(built-in)时链接器整段丢掉
 * （因为 built-in 永不卸载）；可卸载模块则保留，rmmod 时调用。
 */
static void __exit hello_exit(void)
{
	printk(KERN_INFO "Goodbye, World! Module unloaded.\n");
}

/* 两个宏把函数指针登记到内核的模块表里——insmod/rmmod 就是从这里找到入口的 */
module_init(hello_init);
module_exit(hello_exit);
