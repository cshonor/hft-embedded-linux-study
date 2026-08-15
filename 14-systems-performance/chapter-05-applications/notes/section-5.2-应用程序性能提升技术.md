## 5.2 应用程序性能提升技术

### I/O 操作与缓存

| 技术 | 原理 | 注意 |
|------|------|------|
| **I/O 块大小** | 大块摊销 syscall / DMA 固定成本 | 过大增加延迟（等凑满 buffer） |
| **Caching** | 重复读走内存副本 | 一致性、失效策略 |
| **Buffering** | 合并多次小写为一次 | 环形缓冲区、批量 flush |

**HFT：**

- 行情：**预分配 mbuf / ring buffer**，热路径零 malloc（→ [15-DPDK 01-Intro ch02](../../../13-dpdk/01-Intro-Book/notes/chapter-02-mbuf与内存池.md)）。
- 日志 / 落盘：**异步写、批量写**，绝不在 tick 路径上 `fprintf`。

### 轮询 vs 事件驱动

| 模式 | 行为 | 适用 |
|------|------|------|
| **忙轮询（spin）** | 100% CPU 等数据 | DPDK PMD、极低延迟 NIC |
| **`poll(2)`** | 线性扫描 fd，O(n) | 少量 fd 尚可 |
| **`epoll` / `kqueue`** | 内核通知就绪 fd | 多连接、中等延迟 |
| **`io_uring`** | 批量异步 I/O | 新一代 Linux 高吞吐 |

**HFT 选型：**

- 组播行情极致延迟 → **DPDK 轮询** 或 **busy-poll**（内核栈）
- 多交易所 TCP 订单通道 → **epoll + 非阻塞**（→ [12-UNP](../../../03.5-unix-network-api/)）

### 并发与锁

| 机制 | 特点 | HFT |
|------|------|-----|
| **多进程** | 隔离好、通信贵 | 行情 / 发单进程分离 |
| **多线程** | 共享内存、需同步 | 同进程内 pipeline |
| **互斥锁 mutex** | 阻塞等待 | 临界区尽量短 |
| **自旋锁 spinlock** | 忙等，适合极短临界区 | 持锁时间必须 << 时间片 |
| **读写锁 rwlock** | 读多写少 | order book 读多写少场景 |
| **锁分片 / 锁哈希** | 降低竞争 | per-symbol 锁 |

**伪共享（False Sharing）：**

- 两个线程写**同一 cache line** 不同变量 → MESI 来回 invalidation，性能暴跌。
- 对策：`alignas(64)` 填充、per-core 计数器、无锁结构分槽。

→ [19-Hennessy Ch2](../../../17-computer-architecture/) MESI · [13-DPDK CROSS-MODULE](../../README.md)

### 其他技术

| 技术 | 作用 |
|------|------|
| **非阻塞 I/O** | 线程不因单 fd 阻塞而睡死 |
| **CPU 亲和性（affinity）** | 线程 / IRQ / 网卡队列同 NUMA、同核，提升 cache 命中 |
| **Huge pages** | 减 TLB miss（DPDK、大堆 Java 都相关） |

→ [05-linux-kernel](../../../05-linux-kernel/) 调度与绑核 · [18-HFT ch05](../../../16-hft-engineering/chapter-05-操作系统内核极致调优.md)

---


### 常见陷阱

1. 过度优化——没 profile 就手动展开循环/内联汇编，编译器已经做得更好且更可维护
2. 伪共享不查——多线程写同一 cache line 的不同字段，cache 一致性协议打穿性能
3. 数据结构不量化——换 hashmap vs red-black tree 不测实际延迟，凭直觉选可能更慢

<details>
<summary>自测题（点击展开）</summary>

1. HFT 应用层优化的最高 ROI 是什么？
   <details><summary>答</summary>消除不必要的工作——去掉冗余拷贝/分支/日志，比优化已有代码更有效</details>
2. 伪共享如何检测和修复？
   <details><summary>答</summary>perf c2c 或查 cache line 对齐——修复用 alignas(64) 填充使各线程数据不在同一 cache line</details>
3. 为什么换数据结构前必须 benchmark？
   <details><summary>答</summary>hashmap 查找 O(1) 但 cache miss 可能比红黑树 O(log n) 更慢——实际延迟取决于 cache 行为不是理论复杂度</details>

</details>


---

← [本章导读](../README.md)
