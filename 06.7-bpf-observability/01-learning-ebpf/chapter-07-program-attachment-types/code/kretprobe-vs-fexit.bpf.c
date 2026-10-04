/* kretprobe-vs-fexit.bpf.c — 返回探针两代的对比（ch07 §7.1 块1 补全）
 *
 * kretprobe（老）：只能拿返回值；想要入参得在 kprobe 里先存 map 再配对
 * fexit（新，5.5+）：入参和返回值同时在手——它是"带返回值的 fentry"
 *
 * 编译: clang -O2 -g -target bpf -D__TARGET_ARCH_x86 \
 *        -I<有vmlinux.h的目录> -I/usr/include/x86_64-linux-gnu \
 *        -c kretprobe-vs-fexit.bpf.c -o kretprobe-vs-fexit.bpf.o
 *
 * 另见块2：uprobe 的 SEC 写法（挂用户态库函数）
 *   SEC("uprobe/usr/lib/aarch64-linux-gnu/libssl.so.3/SSL_write")
 *   —— 路径里带版本号，升级库就失效；生产建议 USDT 或运行时解析路径
 */
#include "vmlinux.h"
#include <bpf/bpf_helpers.h>
#include <bpf/bpf_tracing.h>
#include <bpf/bpf_core_read.h>

/* ① kretprobe：上下文里只有返回值，入参拿不到 */
SEC("kretprobe/do_unlinkat")
int BPF_KRETPROBE(do_unlinkat_exit, long ret)
{
    bpf_printk("do_unlinkat ret=%ld（文件名？kretprobe 拿不到）", ret);
    return 0;
}

/* ② fexit：入参（dfd, name）与返回值（ret）同时在手，一次搞定
 *
 * ⚠ 内核演进实录（2026-10，本机 7.0.0-38）：
 *   struct filename 的 name 字段被挪进了匿名嵌入的 __filename_head，
 *   BPF_CORE_READ(name, name) 编译报 "no member named 'name'"。
 *   这正是 CO-RE 存在的理由——但匿名嵌入时连 CO-RE 也要换写法：
 *   匿名成员在 offset 0，直接按子结构类型读，重定位交给 BTF。 */
SEC("fexit/do_unlinkat")
int BPF_PROG(do_unlinkat_exit2, int dfd, struct filename *name, long ret)
{
    const char *p;
    bpf_core_read(&p, sizeof(p), &((struct __filename_head *)name)->name);
    bpf_printk("do_unlinkat dfd=%d name=%s ret=%ld", dfd, p, ret);
    return 0;
}

char LICENSE[] SEC("license") = "Dual BSD/GPL";
