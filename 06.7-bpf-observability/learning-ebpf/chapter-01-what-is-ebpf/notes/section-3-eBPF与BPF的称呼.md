# eBPF 与 BPF 的称呼

- 内核源码和编程界面统一用 **BPF**：`bpf()`、`bpf_` 前缀 helper、`BPF_PROG_TYPE_*`
- 内核社区之外流行 **eBPF**（ebpf.io、eBPF Foundation）
- 两者如今可互换——现代内核全都支持"extended"部分
