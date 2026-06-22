## 1.9 重要主题

### 1.9.1 阿姆达尔定律 (Amdahl's Law)

**公式（加速比）：**

\[
S = \frac{1}{(1 - p) + \frac{p}{k}}
\]

- \(p\) — 可并行/可优化部分占整体时间的比例  
- \(k\) — 该部分加速倍数  

**直觉：** 若 90% 时间在一个无法优化的串行段，整体最多快 10 倍，**再优化剩下 10% 收益有限**。

| 场景 | 教训 |
|------|------|
| 优化只占 1% 时间的函数 10× | 整体几乎不变 |
| 行情解析占 60%，优化 2× | 整体显著变快 — **先 profile 找 p** |

**HFT：** 端到端延迟 = 收包 + 解码 + 策略 + 发单 + **排队/内核/网卡**。用 [11-HFT ch10](../../../11-HFT-Low-Latency-Practice/chapter-10-延迟测量与基准压测/) 或 span 分解找 **最大 p**，再动刀。

→ 方法论：[02-SysPerf Ch 2 延迟分解](../../../02-Systems-Performance-2nd/chapter-02-methodologies/)

### 1.9.2 并发和并行 (Concurrency and Parallelism)

| 概念 | 含义 |
|------|------|
| **并发 (concurrency)** | 多个任务在时间上交替推进（可单核） |
| **并行 (parallelism)** | 同一时刻多核同时执行 |

- **线程级并行** — 多核跑多线程
- **指令级并行 (ILP)** — 单核流水线/超标量（→ [Ch 4](../../chapter-04-processor-architecture/)、Hennessy）

**HFT：** 并发正确性（锁、内存序）与并行吞吐（绑核、无共享）往往 **同时** 要 — [Ch 12](../../chapter-12-并发编程.md)。

### 1.9.3 计算机系统中抽象的重要性

层层抽象隐藏细节，让上层 **可组合、可推理**：

```
C 源码 → ISA 指令集 → 微架构 → 逻辑门
进程/VM/文件 → syscall → 内核 → 硬件
socket API → TCP/IP → 以太网帧
```

**好处：** 换 CPU、换磁盘、换网卡，应用大多不用重写。  
**代价：** 抽象 leak 时难排查（page fault、cache、tail latency）。

**HFT 态度：**

- **默认用抽象**（libc、内核栈）写非热路径
- **热路径 selectively 打破抽象**（ hugepage、mlock、busy poll、DPDK）— 知道自己在跳过哪一层

---

## 1.10 小结（原书）

本章用 **hello** 串起：编译链接、CPU/内存/cache、OS 四大抽象、网络、阿姆达尔与并发。后续各章 **分别加厚** 其中一层 — 按 [本章 README 的 HFT 捷径](../README.md#hft-精读捷径) 选读即可。

---

← [本章导读](../README.md)
