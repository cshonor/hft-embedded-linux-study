/* hello-xdp.bpf.c — XDP 版 Hello World（ch03 §3.1 块2）
 * 出自《Learning eBPF》第 3 章：第一个 libbpf 风格（非 BCC）程序
 *
 * 与 BCC 的区别：BCC 在用户态现场编译 C 字符串；
 * libbpf 风格是离线编译成 .o（ELF），再由用户态 loader 加载。
 *
 * 编译: clang -O2 -g -target bpf -c hello-xdp.bpf.c -o hello-xdp.bpf.o
 * 加载（需 root）:
 *   sudo ip link set dev lo xdpgeneric obj hello-xdp.bpf.o sec xdp
 *   sudo cat /sys/kernel/tracing/trace_pipe     # 看 Hello World 计数
 *   sudo ip link set dev lo xdpgeneric off      # 卸载
 * （lo 只支持 xdpgeneric；物理网卡用 xdp/natvie 模式）
 */
#include <linux/bpf.h>
#include <bpf/bpf_helpers.h>

int counter = 0;                       // 全局变量（libbpf 会把它变成 .data map，见 §3.3）

SEC("xdp")                             // ELF 段名 = 程序类型标记
int hello(void *ctx) {
    bpf_printk("Hello World %d", counter);
    counter++;
    return XDP_PASS;                   // 裁决：正常继续处理（其他：DROP/TX/REDIRECT/ABORTED）
}

char LICENSE[] SEC("license") = "Dual BSD/GPL";   // 没这行 verifier 拒绝部分 helper
