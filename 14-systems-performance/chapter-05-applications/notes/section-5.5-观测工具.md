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

→ 完整脚本库：[17-BPF](../../../15-bpf-observability/) · 本仓库附录 C

---


### 常见陷阱

1. pidstat 不指定 TID——多线程程序只看进程级数据，热路径线程被其他线程平均掉
2. perf record 不加 -g——不加 -g 只采到当前函数采不到调用栈，无法做火焰图
3. uprobe 生产直接挂——uprobe 有开销（每次调用陷入），热路径高频函数可能显著增延迟

<details>
<summary>自测题（点击展开）</summary>

1. pidstat 查看 HFT 多线程程序要注意什么？
   <details><summary>答</summary>用 -t 指定 TID——进程级数据会把热路径线程和 housekeeping 线程平均掉</details>
2. perf record 为什么要加 -g？
   <details><summary>答</summary>-g 采集调用栈——不加只能看到当前函数，无法做火焰图定位调用链热点</details>
3. uprobe 在生产环境的注意事项？
   <details><summary>答</summary>有每次调用的开销——热路径高频函数会显著增延迟，应限时长或用 USDT 替代</details>

</details>


---

← [本章导读](../README.md)
