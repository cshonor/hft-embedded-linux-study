/* verifier-fail-bounds.bpf.c — 反例②：边界检查差一格（ch06 §6.2 块2）
 *
 * message[12]，下标合法范围 0..11；
 * `c <= sizeof(message)` 在 c==12 时放行了 message[12] —— 越界！
 * verifier 做范围传播：发现 c 的上界可达 12 → 拒绝。
 *
 * 正确写法: c < sizeof(message)
 */
#include "vmlinux.h"
#include <bpf/bpf_helpers.h>
#include <bpf/bpf_tracing.h>
#include <bpf/bpf_core_read.h>

SEC("ksyscall/execve")
int BPF_KPROBE_SYSCALL(hello, const char *pathname)
{
    const char message[12] = "Hello World";
    u32 c = (u32)(bpf_get_current_pid_tgid() & 0xF);   /* 0..15，运行时才知 */
    char a = 0;

    if (c <= sizeof(message)) {     /* ← c==12 时越界！verifier 拒绝 */
        a = message[c];
    }
    bpf_printk("byte: %d", a);
    return 0;
}

char LICENSE[] SEC("license") = "Dual BSD/GPL";
