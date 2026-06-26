## 5.5 观测工具

### 工具集概览

| 工具 | 类型 | 用途 |
|------|------|------|
| **`strace`** | syscall 追踪 | 开发/debug；**生产慎用**（开销大） |
| **`perf`** | 采样剖析 | CPU 火焰图、PMC、部分 trace |
| **BCC `profile`** | BPF CPU 栈 | 全栈、内核+用户 |
| **BCC/bpftrace `offcputime`** | Off-CPU 栈 | 阻塞分析 |
| **`execsnoop`** | 追踪 exec | 意外子进程 / 脚本调用 |
| **`syscount`** | syscall 计数 | 热路径 syscall 种类与频率 |
| **应用层 USDT / 静态探针** | 自定义 tracepoint | 业务阶段 span |

→ [Ch 4 观测工具](../../chapter-04-observability-tools/) · [附录 C bpftrace](../../appendix-C-bpftrace单行命令.md)

### bpftrace 示例（Off-CPU 思路）

```bash
# 需 BCC offcputime 或等价脚本；概念：采样「可运行但未运行」的栈
# 生产环境优先用预装 BCC 脚本，限时长运行
sudo offcputime-bpfcc -p $(pidof strategy) 30
```

→ 完整脚本库：[04-BPF](../../../04-BPF-Performance-Tools/) · 本仓库附录 C

---


---

← [本章导读](../README.md)
