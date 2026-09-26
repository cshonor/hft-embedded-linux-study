/*
 * hello.c — 最小可加载内核模块（LKM）
 *
 * 用途：验证「真实的 Linux 环境」这条链的最后一环 —— 能不能 insmod。
 * Windows/MSVC 编不出 .ko；WSL2 默认内核也不给加载。这个只在真 Linux 上跑得通。
 *
 * 文中的实测来自 Raspberry Pi 5（Debian 13 aarch64，kernel 6.18.39+rpt-rpi-2712）。
 */
#include <linux/module.h>       /* MODULE_LICENSE / module_init / module_exit */
#include <linux/kernel.h>       /* pr_info —— 内核态 printf */
#include <linux/init.h>         /* __init / __exit */

/*
 * __init：初始化完成后这块内存会被回收（只跑一次的东西用它）
 * 注意这是 GNU C 的 __attribute__((section(".init.text"))) —— 又是 GNU 扩展
 */
static int __init hello_init(void)
{
    pr_info("hello_ko: module loaded, running on real Linux\n");
    return 0;                   /* 返回 0 = 加载成功；负数 = 加载失败 */
}

static void __exit hello_exit(void)
{
    pr_info("hello_ko: module unloaded\n");
}

module_init(hello_init);
module_exit(hello_exit);

MODULE_LICENSE("GPL");          /* 不写会 taint kernel，警告 + 部分符号不可用 */
MODULE_AUTHOR("wzp");
MODULE_DESCRIPTION("Minimal LKM to verify insmod works");
