# 第10章 日志、性能测量与极限优化

> **原书第 7 章 · HFT Optimization – Logging, Performance, and Networking**  
> **Kernel Bypass · mmap IPC · 异步日志 · Tick-to-Trade 测量**

← [chapter-06 动态网络](./chapter-06-低延迟网络与协议优化.md) · [chapter-07 无锁环](./chapter-07-无锁数据结构与内存布局.md)

---

## 本章定位

原书 **Ch7** 将优化从 **代码/数据结构** 扩展到：

- **OS 内核**（Bypass）
- **跨地域物理网络**（微波）
- **日志移出热点**
- **科学测量**（TTT / T2T）

本仓库 **分散落地**：Bypass → [Ch5](./chapter-05-操作系统内核极致调优.md) · 网络 → [Ch6](./chapter-06-低延迟网络与协议优化.md) · mmap/环 → [Ch7](./chapter-07-无锁数据结构与内存布局.md) · **本章 = 日志 + 测量总纲**。

---

## 1. 内核旁路 (Kernel Bypass)

### 传统路径的代价

用户态程序 ↔ **syscall/IRQ** ↔ 内核 ↔ NIC — [chapter-06 §5](./chapter-06-低延迟网络与协议优化.md#5-数据包生命周期kernel-路径)：

- **上下文切换**
- **sk_buff 分配 · 头部分解 · socket 队列 · 拷贝**

### Bypass 核心

| 机制 | 说明 |
|------|------|
| **User-space spinning** | 交易线程 **100% poll** NIC ring — **无阻塞 · 无 IRQ 等待** |
| **Zero copy** | DMA 数据 **映射/直达** 用户缓冲 — **无 memcpy** |

| 产品（示例） | |
|--------------|---|
| **OpenOnload** | Solarflare / AMD |
| **VMA** | Mellanox / NVIDIA |
| **DPDK** | 通用 PMD |

**效果（书中量级）：** UDP/TCP 读写 **1.5–10 μs → 0.5–2 μs**。

→ 实操 [chapter-05 §3](./chapter-05-操作系统内核极致调优.md#3-减少阻塞kernel-bypass) · [14-DPDK](../14-DPDK-Low-Latency-Network/)

---

## 2. 内存映射文件 (mmap) 与 IPC

```cpp
void* addr = mmap(nullptr, size, PROT_READ|PROT_WRITE,
                   MAP_SHARED, fd, 0);
```

| 用途 | HFT |
|------|-----|
| **持久化** | 大文件 **随机访问** 快于 read/write 循环 |
| **非持久化 IPC** | **`/dev/shm` 或 memfd** + **无锁环** — Gateway ↔ Strategy **零拷贝** |

**模式：**

```
进程 A 写 ring slot → 进程 B acquire/release
共享区 = mmap 的同一块物理页
```

→ [chapter-07 §2 无锁环](./chapter-07-无锁数据结构与内存布局.md#2-无锁-fifo-队列) · [08-TLPI mmap](../08-The-Linux-Programming-Interface/)

---

## 3. 跨数据中心：微波与空芯光纤

| 介质 | 特点 |
|------|------|
| **Microwave** | 空气光速 **> 玻璃 ~50%** · **直线** · **最快** · **带宽低 · 天气敏感** |
| **Hollow fiber** | 芯内 **空气通道** · 快于实心光纤 · 常接 **DC ↔ 微波塔**（数百码） |

**场景：** 芝加哥 ↔ 纽约 **latency arbitrage** — 与 [chapter-03 共址](./chapter-03-订单簿深度与行情解析.md#5-共址与市场数据) **互补**（机柜内 μs vs 城际 ms）。

→ [chapter-06 §7](./chapter-06-低延迟网络与协议优化.md#7-广域链路补充)

---

## 4. 移出关键路径：异步日志

### 为何同步日志致命

| 操作 | 问题 |
|------|------|
| **`printf` / fmt** | 字符串格式化 **慢** |
| **sync 写盘** | **ms 级** · 可能 **阻塞** |

**热点路径禁止同步日志。**

### 异步架构

```
Critical Path                    Background Core
─────────────                    ───────────────
binary event ──► lock-free ring ──► Logger thread
  (POD, 固定大小)                      │
                                       ├─ 格式化
                                       └─ 批量 fsync
```

| 要点 | 说明 |
|------|------|
| **二进制 blob** | 热点只 **memcpy 固定 struct** |
| **无锁环** | [chapter-07](./chapter-07-无锁数据结构与内存布局.md) |
| **专属核** | Logger **不抢** Gateway/Strategy 核 |
| **mmap 缓冲** | 可选 **环形文件映射** 批量刷盘 |

**原则：** 格式化与磁盘 I/O **永不** 在 T2T 路径上执行。

---

## 5. 精确性能测量与基准测试

### 消除非确定性（测前必做）

| 层 | 关闭/固定 |
|----|-----------|
| **BIOS** | Turbo · HT · C-states |
| **OS** | **ASLR**（基准时可关）· 无关 IRQ |
| **负载** | **isolcpus** 与生产一致 |

否则 **p99 不可复现** — 优化无从谈起。

→ [chapter-05](./chapter-05-操作系统内核极致调优.md) · [chapter-04 §2](./chapter-04-硬件选型与服务器配置.md)

### 时间戳工具

| 工具 | 说明 |
|------|------|
| **`gettimeofday`** | 常 **syscall** — 测微基准 **慎用** |
| **`rdtsc` / `rdtscp`** | CPU 周期 — **inline** · 需 **频率校准/invariant TSC** |
| **`std::chrono::high_resolution_clock`** | C++11 · 实现可能仍 syscall — **benchmark 验证** |
| **NIC PHC 硬件戳** | **TTT 金标准** — [chapter-06 §6 PTP](./chapter-06-低延迟网络与协议优化.md#6-监控与时间同步) |

```cpp
inline uint64_t rdtsc() {
    unsigned hi, lo;
    __asm__ volatile("rdtsc" : "=a"(lo), "=d"(hi));
    return ((uint64_t)hi << 32) | lo;
}
```

### 端到端 Tick-to-Trade (TTT / T2T)

**终极指标：** 行情进 NIC → 订单出 NIC。

```
t1  NIC RX hardware timestamp
t2  Gateway parse out
t3  Book update done
t4  Strategy signal
t5  OMS risk pass
t6  Gateway OUT serialize
t7  NIC TX submit
…
T2T = t7 - t1
```

| 实践 | 说明 |
|------|------|
| **每 hop 探针** | 定位 **瓶颈组件** |
| **p50/p99/p999** | 尾延迟 · 非仅平均 |
| **Replay 压测** | 生产 pcap **离线** — [chapter-01 实战](./chapter-01-高频交易基础与生态.md#实战启动建议) |
| **探针开销** | 测量工具 **本身** 要 **inline / 采样** |

→ [03-Systems-Performance](../03-Systems-Performance-2nd/)

---

## 本章小结

| 原书 Ch7 主题 | 手段 |
|---------------|------|
| **Bypass** | Spin poll · Zero copy · 0.5–2 μs |
| **mmap** | 共享内存 IPC + 环 |
| **广域网** | 微波 / 空芯光纤 |
| **日志** | 无锁环 → 后台格式化 |
| **测量** | 关 jitter 源 · rdtsc/PHC · **T2T 分段** |

**性能优化推到物理极限后** → 语言层：[chapter-08 C++ 微秒征途（原书 Ch8）](./chapter-08-超低延迟核心引擎开发.md)

---

## 原书章节对照

| 原书 | 本仓库 |
|------|--------|
| Ch7 §1 Bypass | Ch5 · Ch6 · **本章 §1** |
| Ch7 §2 mmap | Ch7 · **本章 §2** |
| Ch7 §3 微波 | Ch6 · **本章 §3** |
| Ch7 §4 日志 | **本章 §4** |
| Ch7 §5 测量 | **本章 §5** |
| Ch8 C++ | **Ch8** |
