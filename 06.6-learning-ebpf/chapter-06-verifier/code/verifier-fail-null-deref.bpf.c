/* verifier-fail-null-deref.bpf.c — 反例①：map 查询不判空（ch06 §6.2 块3）
 *
 * 编译能通过；加载时 verifier 拒绝：
 *   "R1 invalid mem access 'map_value_or_null'"
 * 原因：bpf_map_lookup_elem 返回类型是 RET_PTR_TO_MAP_VALUE_OR_NULL（块1），
 *       verifier 要求任何使用前先排除 NULL 分支。
 *
 * 对照: verifier-pass-fixed.bpf.c
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
    char a = p[0];              /* ← 编译通过，验证失败：p 可能是 NULL */
    bpf_printk("first byte: %d", a);
    return 0;
}

char LICENSE[] SEC("license") = "Dual BSD/GPL";
