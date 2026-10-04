/* lsm-path-chmod.bpf.c — LSM 钩子审计 chmod（ch09 §9.1 块1 补全）
 * 出自《Learning eBPF》第 9 章：BPF LSM 把"安全策略"也变成了可编程事件
 *
 * LSM 程序的特殊性：返回值非零 = **拒绝本次操作**（不只是观测，是管控）
 * 前置：内核需开 CONFIG_BPF_LSM，且 lsm= 启动参数含 bpf
 *   检查: cat /sys/kernel/security/lsm（有 bpf 才可用）
 *
 * 编译: clang -O2 -g -target bpf -D__TARGET_ARCH_x86 \
 *        -I<有vmlinux.h的目录> -I/usr/include/x86_64-linux-gnu \
 *        -c lsm-path-chmod.bpf.c -o lsm-path-chmod.o
 * 加载: sudo bpftool prog load lsm-path-chmod.o /sys/fs/bpf/path_chmod
 */
#include "vmlinux.h"
#include <bpf/bpf_helpers.h>
#include <bpf/bpf_tracing.h>
#include <bpf/bpf_core_read.h>

SEC("lsm/path_chmod")
int BPF_PROG(path_chmod, const struct path *path, umode_t mode)
{
    /* path->dentry->d_name.name：三层指针穿透，CO-RE 逐层重定位
     * ⚠ 内核演进：书上用 d_iname；新内核（≥6.8 前后）dentry 改名 d_name(struct qstr)，
     *   文件名指针在 d_name.name —— CO-RE 写法随之调整（本机 7.0 实测） */
    struct dentry *de = BPF_CORE_READ(path, dentry);
    const unsigned char *iname = BPF_CORE_READ(de, d_name.name);
    bpf_printk("chmod: %s mode=%o (pid=%d)\n", iname, mode,
               (int)(bpf_get_current_pid_tgid() >> 32));

    /* 演示管控：把 0777 的 chmod 拒掉（世界可写可执行是经典事故源） */
    if ((mode & 0777) == 0777) {
        bpf_printk("  → 拒绝 chmod 777\n");
        return -1;   /* 非零 = 拒绝本次 chmod（调用方得到 EPERM） */
    }
    return 0;
}

char LICENSE[] SEC("license") = "GPL";   /* LSM 程序要求 GPL（用了 gpl_only helper） */
