/* hello-bpf2bpf.bpf.c — BPF-to-BPF 函数调用（ch03 §3.3 块3）
 *
 * noinline 的意义：告诉编译器"不要内联"，生成真正的 BPF-to-BPF 调用指令
 * （内核 4.16+ 支持；之前只能 __always_inline 复制展开）。
 * 用 llvm-objdump -d 可以看到 call 指令指向另一个 BPF 函数。
 *
 * 编译: clang -O2 -g -target bpf -c hello-bpf2bpf.bpf.c -o hello-bpf2bpf.bpf.o
 * 反汇编看调用: llvm-objdump -d hello-bpf2bpf.bpf.o
 */
#include <linux/bpf.h>
#include <bpf/bpf_helpers.h>

struct bpf_raw_tracepoint_args;   /* 前向声明（完整定义在 vmlinux.h，本例只需指针） */

static __attribute((noinline)) int get_opcode(struct bpf_raw_tracepoint_args *ctx) {
    /* args[1] = syscall 号；用 helper 读内核内存更稳，这里保持书上的直读写法 */
    long *args = (long *)ctx;
    return (int)(args[1] & 0xFFFFFFFF);
}

SEC("raw_tp")
int hello(struct bpf_raw_tracepoint_args *ctx) {
    int opcode = get_opcode(ctx);
    bpf_printk("Syscall: %d", opcode);
    return 0;
}

char LICENSE[] SEC("license") = "Dual BSD/GPL";
