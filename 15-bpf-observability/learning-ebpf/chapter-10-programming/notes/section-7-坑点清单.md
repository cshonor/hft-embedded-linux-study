# 坑点清单

1. **bpftrace/BCC 基于 syscall 入口的工具有 TOCTOU**（第 9 章）——观测可用，别当安全防线
2. gobpf 已停止维护——新项目别选
3. libbpfgo 的 CGo 边界（"cgo is not Go"）可能带来性能/构建问题——对延迟敏感选纯 Go 的 cilium/ebpf
4. BCC 的"类 C 方言"不是标准 C：`BPF_RINGBUF_OUTPUT`、对象方法等宏只在 BCC 里有——代码不可移植到 libbpf
5. cilium/ebpf 生成两套字节码（大端 bpfeb / 小端 bpfel）——交叉编译时确认架构选对了 `.o`
6. `bpf_stats_enabled` 统计本身有开销，测完记得关
7. 内核侧 C 用户侧也 C 时，用户态代码没有验证器兜底——map 读写、缓冲区处理全靠自己
