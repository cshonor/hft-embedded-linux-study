/* hello-buffer-config.bpf.c — CO-RE 内核侧（ch05 §5.3 块3/4/5/6 组合补全）
 * 出自《Learning eBPF》第 5 章 hello-buffer-config 例子的完整版
 *
 * 编译（scripts/build-core.sh 自动做）：
 *   bpftool btf dump file /sys/kernel/btf/vmlinux format c > vmlinux.h
 *   clang -O2 -g -target bpf -D__TARGET_ARCH_x86 -I. -I/usr/include/x86_64-linux-gnu \
 *         -c hello-buffer-config.bpf.c -o hello-buffer-config.bpf.o
 * 生成 skeleton（用户态 loader 用）：
 *   bpftool gen skeleton hello-buffer-config.bpf.o > hello-buffer-config.skel.h
 */
#include "vmlinux.h"            // 内核全部类型（构建时从本机 BTF 生成）
#include <bpf/bpf_helpers.h>    // helper 函数与 map 宏
#include <bpf/bpf_tracing.h>    // BPF_KPROBE_SYSCALL 等
#include <bpf/bpf_core_read.h>  // CO-RE 读内存封装
#include "hello-buffer-config.h"

/* ---- map 声明（BTF 风格 SEC(".maps")，块4） ---- */
struct {
    __uint(type, BPF_MAP_TYPE_PERF_EVENT_ARRAY);
    __uint(key_size, sizeof(u32));
    __uint(value_size, sizeof(u32));
} output SEC(".maps");

struct {
    __uint(type, BPF_MAP_TYPE_HASH);
    __uint(max_entries, 10240);
    __type(key, u32);
    __type(value, struct user_msg_t);
} my_config SEC(".maps");

/* ---- SEC 附加点三种写法（块5） ----
 * SEC("kprobe")                    只声明类型，附加点留给用户态
 * SEC("kprobe/__x64_sys_execve")   指定具体函数（架构相关！x86_64 前缀 __x64_sys_）
 * SEC("ksyscall/execve")           libbpf 自动解析架构相关名 —— 可移植推荐
 */

const char message[12] = "Hello World";

SEC("ksyscall/execve")
int BPF_KPROBE_SYSCALL(hello, const char *pathname)
{
    struct data_t data = {};
    struct user_msg_t *p;

    data.pid = bpf_get_current_pid_tgid() >> 32;
    data.uid = bpf_get_current_uid_gid() & 0xFFFFFFFF;
    bpf_get_current_comm(&data.command, sizeof(data.command));
    bpf_probe_read_user_str(&data.path, sizeof(data.path), pathname);

    /* 查用户态下发的配置：这个 UID 有没有自定义消息 */
    p = bpf_map_lookup_elem(&my_config, &data.uid);
    if (p != 0)
        bpf_probe_read_kernel(&data.message, sizeof(data.message), p->message);
    else
        bpf_probe_read_kernel(&data.message, sizeof(data.message), message);

    bpf_perf_event_output(ctx, &output, BPF_F_CURRENT_CPU, &data, sizeof(data));
    return 0;
}

char LICENSE[] SEC("license") = "Dual BSD/GPL";
