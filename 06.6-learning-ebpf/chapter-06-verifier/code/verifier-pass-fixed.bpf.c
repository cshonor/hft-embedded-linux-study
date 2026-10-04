/* verifier-pass-fixed.bpf.c — 修正版：判空 + 严格小于（ch06 §6.2 对照组）
 *
 * 与两个反例一一对应：
 *   ① map 查询先判 NULL 再用（verifier 沿分支确认指针非空）
 *   ② 边界用 < 而不是 <=（范围传播确认上界 11）
 */
#include "vmlinux.h"
#include <bpf/bpf_helpers.h>
#include <bpf/bpf_tracing.h>
#include <bpf/bpf_core_read.h>

struct {
    __uint(type, BPF_MAP_TYPE_HASH);
    __uint(max_entries, 1024);
    __type(key, u32);
    __type(value, char[12]);
} my_config SEC(".maps");

SEC("ksyscall/execve")
int BPF_KPROBE_SYSCALL(hello, const char *pathname)
{
    u32 uid = bpf_get_current_uid_gid() & 0xFFFFFFFF;
    char *p = bpf_map_lookup_elem(&my_config, &uid);
    if (p == 0)                 /* ① 判空：NULL 分支直接返回 */
        return 0;
    bpf_printk("first byte: %d", p[0]);

    const char message[12] = "Hello World";
    u32 c = (u32)(bpf_get_current_pid_tgid() & 0xF);
    if (c < sizeof(message))    /* ② 严格小于：c 上界 11，verifier 通过 */
        bpf_printk("byte: %d", message[c]);
    return 0;
}

char LICENSE[] SEC("license") = "Dual BSD/GPL";
