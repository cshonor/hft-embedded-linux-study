# 第7章 HFT 优化：无锁与内存

> **原书第 6 章 §2–§3 · Lock-free · Memory Pool · Cache-friendly**  
> （原书 Ch7「日志/网络」中 Bypass 已见 [chapter-06](./chapter-06-低延迟网络与协议优化.md)）

← 上下文切换：[chapter-05](./chapter-05-操作系统内核极致调优.md) · 总览：[chapter-01 §3](./chapter-01-高频交易基础与生态.md#3-无锁数据结构与-ipc)

---

## 本章定位

原书 **Ch6** 后两支柱：

1. **无锁数据结构** — 进程/线程间 **零阻塞** 传数据  
2. **内存预取与预分配** — **缓存友好** · **禁热点 malloc**

**软件层压榨 μs/ns 的核心秘诀。**

---

## 1. 传统锁的致命弱点

| 问题 | 说明 |
|------|------|
| **Lock contention** | 多线程 **抢锁** — 串行化 |
| **Deadlock** | 循环等待 |
| **Priority inversion** | 低优先级 **持锁** → 高优先级 **被挡** |
| **Convoying（护航）** | 醒来的线程 **排队抢锁** — 全员慢 |
| **阻塞 + 切换** | 等锁 → **sleep** → [chapter-05 上下文切换](./chapter-05-操作系统内核极致调优.md) |

**HFT 结论：** 关键路径 **不用 mutex** — 用 **无锁环** 或 **SPSC 单写单读**。

---

## 2. 无锁 FIFO 队列

### SPSC / MPMC

```
[ slot0 | slot1 | … | slotN-1 ]   // 预分配固定数组
   ^last_write          ^last_read
```

| 模式 | 适用 |
|------|------|
| **SPSC** | 单 Gateway → 单 Strategy — **最简单** |
| **MPMC** | 多源行情 · 需 **原子 CAS** 或 **Disruptor 序列号** |

**实现要点：**

- **原子操作 / 自旋** — 线程 **busy-wait** 短临界区 · **不 yield**（避免切换）
- **release/acquire** 发布 slot 数据 — 见 §4

### 应用场景

| 队列 | 作用 |
|------|------|
| **Gateway → Strategy** | 行情 **fan-out** |
| **Strategy → Logger** | **移出热点** — 异步落盘/统计 |
| **OMS → 审计** | 非关键路径 |

→ [chapter-29 PipeDescriptor 对照](./chapter-29-ipc/notes/section-3-管道机制与PipeDescriptor.md) — 生产可用 **共享内存环** 替代消息队列

---

## 3. 内存预分配与缓存友好

### 延迟数量级

| 访问 | 约 |
|------|-----|
| **L1** | ~0.5 ns |
| **RAM** | ~60 ns |

**目标：** 热点数据 **常驻 L1/L2**。

### 空间与时间局部性

| 原则 | 实践 |
|------|------|
| **Spatial** | **`std::vector` / 数组** > `std::list` 指针追逐 |
| **Temporal** | 同一 order/book 结构 **反复访问** |
| **False sharing** | `read_idx` / `write_idx` **不同 cache line**（`alignas(64)`） |

### 消除不可预测分支

| 避免 | 原因 |
|------|------|
| **虚函数** | vtable · **阻碍 inline** · 分支预测失败 → **流水线清空** |
| **深 if-else** | misprediction 代价 |

**替代：** **CRTP / 模板** — [chapter-08 §5](./chapter-08-超低延迟核心引擎开发.md#5-c-引擎编码规范热点路径) · [chapter-04 §3 静态链接](./chapter-04-硬件选型与服务器配置.md#3-编译器与链接)

### 禁止热点动态分配

| `malloc`/`new` 问题 | |
|---------------------|---|
| 堆 **遍历找块** | 延迟不确定 |
| **Heap fragmentation** | 长期运行 **破坏 locality** |

**终极方案：**

| 策略 | 说明 |
|------|------|
| **栈缓冲** | 小对象、生命周期短 |
| **Memory Pool** | 启动时 **一大块连续内存** · 自定义 **free-list / index stack** |
| **对象复用** | Order/Event **placement new** 回收到池 |

```cpp
// 示意：启动时
pool = mmap_huge(N * sizeof(MarketEvent));
free_stack = {0..N-1};

MarketEvent* acquire() {
    int i = free_stack.pop();
    return &pool[i];
}
```

→ [chapter-05 Huge Pages](./chapter-05-操作系统内核极致调优.md#5-huge-pages与-tlb)

---

## 3. 共享内存 IPC：`mmap` + 无锁环

| API | HFT 用法 |
|-----|----------|
| **`mmap(MAP_SHARED)`** | 文件 / **`/dev/shm`** / memfd → **多进程同物理页** |
| **非持久化** | 纯 RAM 环 · 不必落盘 |
| **+ FIFO** | Gateway 写 · Strategy 读 — **零拷贝 IPC** |

→ 原书 Ch7 §2：[chapter-10 §2](./chapter-10-延迟测量与基准压测.md#2-内存映射文件-mmap-与-ipc)

---

## 4. C++ 内存序（无锁必备）

```cpp
// 生产者
slot[i] = event;
write_idx.store(i + 1, std::memory_order_release);

// 消费者
while (read_idx.load(std::memory_order_acquire) == write_idx) { /* spin */ }
consume(slot[read_idx]);
```

| 顺序 | 使用 |
|------|------|
| **release / acquire** | **发布-消费** 同步 |
| **relaxed** | 纯统计 |
| **seq_cst** | 默认过重 |

---

## 5. LMAX Disruptor（参考架构）

| 概念 | 说明 |
|------|------|
| **Ring buffer** | 预分配 Event **数组** |
| **Sequence** | 全局序号 · **无锁 claim** |
| **Batching** | 消费者 **批量** 处理降开销 |

---

## 本章小结

| 支柱 | 手段 |
|------|------|
| **无锁** | SPSC/MPMC 环 · 原子序 · 日志 **移出热点** |
| **内存** | Pool · vector · 无虚函数 · 无热点 new |
| **+ Ch5** | 绑核 · Bypass · 少切换 |

**三合一（原书 Ch6）：** 绑核 **+** 无锁 **+** 内存池 = **软件延迟压榨极限**。

**下一步：** [chapter-08 核心引擎](./chapter-08-超低延迟核心引擎开发.md) · [chapter-10 测量](./chapter-10-延迟测量与基准压测.md)

---

## 原书章节对照

| 原书 | 本仓库 |
|------|--------|
| Ch6 §1 上下文切换 | **Ch5** |
| Ch6 §2–3 无锁/内存 | **本章 Ch7** |
| Ch7 日志/性能/网络 | **Ch10** · Ch5/6/7 分节 |
