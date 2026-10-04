/* ssl-sniff.bpf.c — uprobe 抓 SSL 明文（ch08 §8.2 块5 补全）
 * 出自《Learning eBPF》第 8 章：SSL_write/SSL_read 的明文都在用户态库边界上
 *
 * 原理：
 *   SSL_write(ssl, buf, num)：entry 时 buf 里就是明文，直接读
 *   SSL_read(ssl, buf)：entry 时 buf 还没数据 → 记下 "SSL* → buf" 映射；
 *                       uretprobe 时返回值=实际字节数，反查 buf 再读
 *
 * 编译: clang -O2 -g -target bpf -D__TARGET_ARCH_x86 \
 *        -I<有vmlinux.h的目录> -I/usr/include/x86_64-linux-gnu \
 *        -c ssl-sniff.bpf.c -o ssl-sniff.o
 * 挂载（需 root，库路径按本机实际）:
 *   SEC 写死路径的局限见 ch07 注释；生产用用户态 loader 解析 libssl 路径再 attach
 */
#include "vmlinux.h"
#include <bpf/bpf_helpers.h>
#include <bpf/bpf_tracing.h>
#include <bpf/bpf_core_read.h>

#define MAX_DATA 256

struct ssl_event {
    __u32 pid;
    __u32 len;
    __u8  rw;                 /* 0=write(出) 1=read(入) */
    char  data[MAX_DATA];
};

struct {
    __uint(type, BPF_MAP_TYPE_PERF_EVENT_ARRAY);
    __uint(key_size, sizeof(__u32));
    __uint(value_size, sizeof(__u32));
} events SEC(".maps");

/* 块5 原样：SSL_read 上下文配对 map（"SSL 指针 → 缓冲区指针"） */
struct {
    __uint(type, BPF_MAP_TYPE_HASH);
    __type(key,   __u64);            /* SSL*（两个探针都能看到） */
    __type(value, __u64);            /* buf 指针（第二参数） */
    __uint(max_entries, 1024);
} ssl_read_context SEC(".maps");

static __always_inline int submit_ssl_data(void *ctx, const void *buf, int num, __u8 rw) {
    if (num <= 0)
        return 0;
    struct ssl_event e = {};
    e.pid = bpf_get_current_pid_tgid() >> 32;
    e.rw  = rw;
    e.len = num > MAX_DATA ? MAX_DATA : num;
    bpf_probe_read_user(e.data, e.len, buf);
    bpf_perf_event_output(ctx, &events, BPF_F_CURRENT_CPU, &e, sizeof(e));
    return 0;
}

SEC("uprobe/SSL_write")
int BPF_KPROBE(ssl_write, const void *ssl, const void *buf, int num) {
    return submit_ssl_data(ctx, buf, num, 0);   /* entry 直接读明文 */
}

SEC("uprobe/SSL_read")
int BPF_KPROBE(ssl_read_enter, const void *ssl, void *buf) {
    /* 只记下 buf 指针，现在还没数据 */
    bpf_map_update_elem(&ssl_read_context, &ssl, &buf, 0);
    return 0;
}

SEC("uretprobe/SSL_read")
int BPF_KPROBE(ssl_read_exit, long ret) {
    /* 以 ssl* 为 key 反查 buf 指针，再从 buf 读 ret 字节 → 明文 */
    __u64 ssl = PT_REGS_PARM1(ctx);
    __u64 *bufp = bpf_map_lookup_elem(&ssl_read_context, &ssl);
    if (bufp == 0)
        return 0;
    __u64 buf = *bufp;
    bpf_map_delete_elem(&ssl_read_context, &ssl);   /* 配对完成，清掉防泄漏 */
    return submit_ssl_data(ctx, (void *)buf, (int)ret, 1);
}

char LICENSE[] SEC("license") = "Dual BSD/GPL";
