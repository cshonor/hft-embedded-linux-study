# XDP 版 Hello World（纯 C）

```c
#include <linux/bpf.h>
#include <bpf/bpf_helpers.h>
int counter = 0;                       // 全局变量
SEC("xdp")                             // ELF 段名 = 程序类型标记
int hello(void *ctx) {
    bpf_printk("Hello World %d", counter);
    counter++;
    return XDP_PASS;                   // 裁决：正常继续处理
}
char LICENSE[] SEC("license") = "Dual BSD/GPL";
```

**要点：**
- 文件名约定 `*.bpf.c` 区分内核态代码与用户态代码
- `SEC("license")` 是**硬性要求**：部分 helper 是 "GPL only"，声明不兼容时验证器直接拒绝；LSM 类程序必须 GPL 兼容
- `bpf_printk`（libbpf 名）/`bpf_trace_printk`（BCC 名）是同一内核函数的封装
- XDP 在包**到达网卡入口**的瞬间触发，可改包内容并给裁决（PASS/DROP/REDIRECT…）
- 部分网卡支持 **XDP 卸载**，程序直接跑在网卡上（DDoS 防护、防火墙、负载均衡利器）
