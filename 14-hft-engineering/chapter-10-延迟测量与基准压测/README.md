# 第10章 延迟测量与基准压测（索引）

> **原书第 7 章 · HFT Optimization – Logging, Performance, and Networking**
> **Kernel Bypass · mmap IPC · 异步日志 · Tick-to-Trade 测量**

← [chapter-06 动态网络](../chapter-06-低延迟网络与协议优化/README.md) · [chapter-07 无锁环](../chapter-07-无锁数据结构与内存布局/README.md)

---

## 本章定位

原书 **Ch7** 将优化从 **代码/数据结构** 扩展到：

- **OS 内核**（Bypass）
- **跨地域物理网络**（微波）
- **日志移出热点**
- **科学测量**（TTT / T2T）

本仓库 **分散落地**：Bypass → [Ch5](../chapter-05-操作系统内核极致调优/README.md) · 网络 → [Ch6](../chapter-06-低延迟网络与协议优化/README.md) · mmap/环 → [Ch7](../chapter-07-无锁数据结构与内存布局/README.md) · **本章 = 日志 + 测量总纲**。

## 小节索引

| 节 | 主题 | 一句话 |
|----|------|--------|
| [10.1](./10.1-Kernel-Bypass.md) | Kernel Bypass | Spin poll + 零拷贝，1.5–10 μs → 0.5–2 μs |
| [10.2](./10.2-mmap与IPC.md) | mmap 与 IPC | 共享物理页 + 无锁环 = 零拷贝 IPC |
| [10.3](./10.3-微波与空芯光纤.md) | 微波与空芯光纤 | 城际 latency arbitrage 的物理介质 |
| [10.4](./10.4-异步日志.md) | 异步日志 🔴 | 二进制 blob 进无锁环，格式化永不进热路径 |
| [10.5](./10.5-精确测量与基准压测.md) | 精确测量与基准压测 🔴 | 消除 jitter 源 · rdtsc/PHC · T2T 分段 |

## 本章小结

| 原书 Ch7 主题 | 手段 |
|---------------|------|
| **Bypass** | Spin poll · Zero copy · 0.5–2 μs |
| **mmap** | 共享内存 IPC + 环 |
| **广域网** | 微波 / 空芯光纤 |
| **日志** | 无锁环 → 后台格式化 |
| **测量** | 关 jitter 源 · rdtsc/PHC · **T2T 分段** |

**性能优化推到物理极限后** → 语言层：[chapter-08 C++ 微秒征途（原书 Ch8）](../chapter-08-超低延迟核心引擎开发/README.md)

## 原书章节对照

| 原书 | 本仓库 |
|------|--------|
| Ch7 §1 Bypass | Ch5 · Ch6 · **本章 10.1** |
| Ch7 §2 mmap | Ch7 · **本章 10.2** |
| Ch7 §3 微波 | Ch6 · **本章 10.3** |
| Ch7 §4 日志 | **本章 10.4** |
| Ch7 §5 测量 | **本章 10.5** |
| Ch8 C++ | **Ch8** |
