# 第7章 无锁数据结构与内存布局

> **Lock-free Ring · Disruptor · Cache Line 对齐**

← 总览：[chapter-01 §3](./chapter-01-高频交易基础与生态.md#3-无锁数据结构与-ipc)

---

## 1. 为何 HFT 禁用锁

| 锁的问题 | 后果 |
|----------|------|
| **阻塞** | 生产者/消费者 **排队** |
| **优先级反转** | 延迟尖刺 |
| **上下文切换** | **μs 级** 惩罚 |
| **死锁风险** | 运维灾难 |

**结论：** 关键路径 **MPMC/MPSC 无锁环** 或 **单写单读** 更简单。

---

## 2. 环形缓冲区（Ring Buffer）

```
[ slot0 | slot1 | … | slotN-1 ]
   ^write              ^read
```

| 设计 | 要点 |
|------|------|
| **连续数组** | **cache friendly** |
| **幂次大小** | `index & (N-1)` 代替 `%` |
| **Cache line padding** | `read_idx` / `write_idx` **分线** 防 false sharing |
| **LMAX Disruptor** | 序列号 + 预分配 Event 槽 |

**典型用途：**

- Gateway IN → Book Builder **market event**
- Strategy → OMS **order intent**
- 热点 → **logging 线程**（异步落盘）

---

## 3. 共享内存 IPC

| 方式 | 说明 |
|------|------|
| **`mmap` 共享环** | 多进程 **零拷贝** 传事件 |
| **固定布局 struct** | **POD** · 无指针（跨进程无效） |
| **版本/序列号** | 检测 **慢读者** 覆盖 |

→ [chapter-01 实战起步](./chapter-01-高频交易基础与生态.md#实战启动建议)

---

## 4. C++ 内存序（无锁必备）

```cpp
// 生产者
data = x;
write_idx.store(i + 1, std::memory_order_release);

// 消费者
while (read_idx.load(std::memory_order_acquire) == write_idx) spin;
```

| 顺序 | 使用 |
|------|------|
| **release / acquire** | 环 slot **发布/消费** |
| **relaxed** | 仅统计计数（无同步需求时） |
| **seq_cst** | 默认过重 — **避免滥用** |

→ [chapter-08 C++ 规范](./chapter-08-超低延迟核心引擎开发.md#5-c-引擎编码规范热点路径)
