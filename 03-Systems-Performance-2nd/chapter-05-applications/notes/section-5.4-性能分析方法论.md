## 5.4 性能分析方法论

### 线程状态分析（Thread State Analysis）

Gregg **排查性能问题的首选框架** — 把线程时间分解为 **9 种状态**：

| 状态 | 含义 | 典型原因 |
|------|------|----------|
| **User** | 用户态执行 | 策略计算、解码、锁内业务逻辑 |
| **Kernel** | 内核态执行 | syscall、协议栈、驱动 |
| **Runnable** | 就绪等 CPU | 调度延迟、run queue 过长 |
| **Swapping** | 换页等待 | 内存不足 — HFT 裸机应 **禁止 swap** |
| **Disk I/O** | 等磁盘 | 日志、checkpoint — 移出热路径 |
| **Net I/O** | 等网络 | recv 阻塞、对端慢 |
| **Sleeping** | 主动 sleep | `sleep`、`cond_wait`、epoll_wait |
| **Lock** | 等锁 | mutex、spin 竞争 |
| **Idle** | 空闲 | 正常；若 hot thread idle 则说明 starvation 或绑核错误 |

**用法：** 对 hot path 线程采样或追踪，看**哪几种状态占比最高** — 占比最高的状态决定下一步用 CPU profile 还是 off-CPU profile。

```
例：行情线程 60% User + 25% Net I/O + 10% Lock + 5% Runnable
  → 先剖 User（算法）+ 查 Lock（竞争）+ Net I/O 是否可旁路（DPDK）
```

→ Ch 2 [延迟分解](../../chapter-02-methodologies/)

### CPU 剖析与 Off-CPU 剖析

| 类型 | 回答什么 | 工具 | 可视化 |
|------|----------|------|--------|
| **CPU Profiling** | 在 CPU 上算了多久、哪个函数 | `perf record`、BPF `profile` | **CPU 火焰图** — 找最宽的「塔」 |
| **Off-CPU Analysis** | 不在 CPU 上时在等什么 | BPF `offcputime`、调度追踪 | **Off-CPU 火焰图** — 找阻塞栈 |

**关键洞察：**

- 只开 CPU profile → 线程大量阻塞在 I/O/锁时，**栈采样几乎采不到**，会误判「CPU 很空所以没问题」。
- **CPU + Off-CPU 一起看**才能解释「时间都去哪了」。

**HFT 火焰图阅读：**

1. CPU 火焰图：策略函数、解码、memcpy 谁最宽？
2. Off-CPU：等锁？等 recv？等 futex？
3. 与 **P99 尖刺**时间窗口对齐（FlameScope 思路，Ch 2）

→ [Ch 13 perf](../../chapter-13-perf/) · [Ch 15 BPF](../../chapter-15-bpf/) · [04-BPF](../../../04-BPF-Performance-Tools/)

### 系统调用与锁分析

| 分析对象 | 工具思路 | 发现 |
|----------|----------|------|
| **Syscall 频率 / 耗时** | `strace -c`、`syscount`、`execsnoop` | 热路径是否 syscall 过多 |
| **Syscall 延迟分布** | BPF 追踪 enter/exit | 哪些 syscall tail 长 |
| **锁竞争** | `perf lock`、`mutrace`、BPF 追踪 mutex | 谁持锁久、谁在等 |

**HFT：** tick 路径上 unexpected 的 `read`/`write`/`malloc`（经 brk/mmap）syscall — 往往是优化突破口。

### 分布式追踪（Distributed Tracing）

微服务架构下，单次请求跨多服务 — 需要 **trace context**（trace id / span id）传递：

```
Gateway → Auth → Order → Matching → Exchange
   |______________ 总延迟 ______________|
        各段 span 可定位最慢服务
```

**HFT 单体 / 低服务化**同样适用 **进程内 span**：

```
recv_ts → decode_ts → book_update_ts → signal_ts → send_ts
```

不必上全套 Jaeger — **关键阶段打 timestamp** 到 ring buffer，离线关联即可。

→ Ch 1 [分布式追踪概念](../../chapter-01-intro/)

---


---

← [本章导读](../README.md)
