# 6. 其他工具（17.5）

> 底本：《BPF之巅》第 17 章，17.5 节（印刷 p755）

其他基于 BPF 的工具：

| 工具 | 定位 |
|---|---|
| **Cilium** | 使用 BPF 在**容器化环境**中应用网络和应用程序安全策略 |
| **Sysdig** | 使用 BPF 扩展**容器**的可观测性 |
| **Android eBPF** | 在 Android 设备上监控和管理设备的网络使用情况 |
| **osquery eBPF** | 公布操作系统信息用于分析和监控；现已支持通过 BPF 监控 kprobes |
| **ply** | 基于 BPF 的**命令行跟踪器**，类似 bpftrace，但**依赖非常少**——非常适合包括嵌入式目标在内的环境；由 Tobias Waldekranz 开发 |

随着 BPF 使用量的增长，将来可能会开发更多基于 BPF 的 GUI 工具。

## HFT 关联

- **ply** 对 HFT 嵌入式侧（行情采集前置机、FPGA 旁的 ARM 控制核）价值最高——无 LLVM/无 BCC 依赖的轻量 BPF 跟踪器，契合本仓库 08-embedded-boot-build 的目标环境
- **Cilium** 是 K8s 环境下用 BPF 取代 iptables 的数据面，若交易集群跑 K8s 可用它降低网络路径抖动

<details>
<summary>自测题</summary>

1. ply 与 bpftrace 相比的特点是什么？适合什么环境？
2. Cilium、Sysdig、Android eBPF、osquery eBPF 各自的主战场？

</details>

<details><summary>参考答案</summary>

1. ply 是类似 bpftrace 的命令行跟踪器，但**依赖非常少**——不需要 LLVM/BCC 那套编译栈（bpftrace 每条脚本要现编译 BPF 字节码，ply 用内置小编译器）。适合**嵌入式目标**等装不下完整工具链的环境。
2. Cilium：容器环境的**网络数据面 + 应用安全策略**（BPF 取代 iptables）；Sysdig：容器**可观测性**扩展；Android eBPF：设备**网络使用监控与管理**；osquery eBPF：把操作系统信息以 SQL 化方式公布用于分析监控，现已支持 BPF 监控 kprobes。

</details>
