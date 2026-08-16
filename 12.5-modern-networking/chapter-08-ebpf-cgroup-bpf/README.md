# Chapter 08: eBPF 与 cgroup BPF

> 来源：Bootlin（eBPF 网络）+ kernel-docs（BPF）+ LWN（XDP-BPF + cgroup BPF）
> 对标：Rosen（无 eBPF，3.x 仅有 classic BPF）

## 小节索引

| # | 文件 | 主题 |
|---|------|------|
| 1 | [ebpf-net-bootlin](notes/01-ebpf-net-bootlin.md) | Bootlin：eBPF 在网络栈的应用、tc-bpf / xdp-bpf |
| 2 | [bpf](notes/02-bpf.md) | kernel-docs：BPF 程序类型、attach 点、helper 函数 |
| 3 | [xdp-bpf](notes/03-xdp-bpf.md) | LWN：XDP BPF 程序、native vs generic、JIT 编译 |
| 4 | [cgroup-bpf](notes/04-cgroup-bpf.md) | LWN：cgroup BPF、sock_ops、connect/load_program |

## HFT 关联

- **XDP BPF**：HFT 可在 XDP hook 运行 BPF 程序做快速过滤/解析，延迟 < 100ns
- **cgroup BPF**：按 cgroup 挂载 BPF 程序，仅对交易进程的 socket 生效，不影响其他进程
- **sock_ops BPF**：监控 socket 状态变化，记录 RTT/丢包，无需修改应用代码
- **JIT 编译**：eBPF JIT 为原生指令，性能接近手写内核代码

## 交叉引用

- `12.5-modern-networking/chapter-05-xdp-architecture/`：XDP hook 是 eBPF 的 attach 点
- `12.5-modern-networking/chapter-09-tc-bpf/`：TC BPF 流量分类
- `06.7-bpf-observability/`：eBPF 可观测性体系
