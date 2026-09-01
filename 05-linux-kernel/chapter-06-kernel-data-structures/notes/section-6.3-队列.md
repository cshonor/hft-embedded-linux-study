## ② 队列 · Queues · `kfifo`

**FIFO** — 典型 **生产者 / 消费者** 模型。

| API（概念） | 作用 |
|-------------|------|
| 创建（静态/动态） | 分配环形缓冲区 |
| **`kfifo_in()`** | 入队 |
| **`kfifo_out()`** | 出队 |
| peek / 查大小 | 窥探队首、当前长度 |

```
生产者（中断/某线程）          消费者（内核线程）
      │ kfifo_in                    │ kfifo_out
      ▼                             ▼
   ┌──────────────────────────────────┐
   │            kfifo 环形缓冲          │
   └──────────────────────────────────┘
```

| 场景 | 说明 |
|------|------|
| 中断里 **入队**、进程上下文 **出队** | 解耦快慢路径 — 需 **同步**（Ch 9） |
| 定长元素 | `kfifo` 按记录大小操作 |

#### SPSC 无锁的机理：回绕用 unsigned 溢出

kfifo 内部 head/tail 是**单调递增的 unsigned int 计数**（不是 0..size-1 的索引），真实位置 = `计数 & (size-1)`：

```
head（生产者写，消费者读）: 总入队字节数 —— 只增
tail（消费者写，生产者读）: 总出队字节数 —— 只增
              │
              ▼ 回绕交给 unsigned 自然溢出（wraparound）
in  = kfifo_in(...)   : 检查 head-tail < size（有空间?）→ 写 & (size-1) 处 → 更新 head
out = kfifo_out(...)  : 检查 head-tail > 0（有数据?）→ 读 & (size-1) 处 → 更新 tail
```

| 性质 | 由谁保证 |
|------|----------|
| head 只被生产者**写**、tail 只被消费者**写** | 单写者 → 无 lost update |
| 满/空判断只需读对方的计数 | 单调计数 + unsigned 减法（溢出安全）→ 判断永远正确 |
| 顺序性 | **kfifo 本身不插内存屏障**——SPSC 语义正确性还需调用方在 head 更新前后加 `smp_wmb()`/`smp_rmb()`（官方 `kfifo` 文档明示：同步由调用者负责） |

> 第三行是最容易翻车的：**"kfifo 是无锁的"只对数据竞争成立，不对内存序成立**。内核里用 kfifo 的代码常在入队后 `smp_wmb()` 再触发消费者（如 wake_up），用户态 rte_ring 的 `ENQUEUE/MEMORY BARRIER/DEQUEUE` 三段注释讲的是同一件事。

#### kfifo API 速查（现代写法）

| 操作 | 函数 | 说明 |
|------|------|------|
| 创建 | `kfifo_alloc(fifo, size, gfp)` | size 会被**向下取到 2 的幂** |
| 声明即定义 | `DECLARE_KFIFO(name, type, size)` + `INIT_KFIFO` | 静态分配，无 GFP |
| 入队 | `kfifo_in(fifo, buf, n)` | 返回实际写入数（可能不满 n） |
| 出队 | `kfifo_out(fifo, buf, n)` | 返回实际读出数 |
| 窥视 | `kfifo_peek(fifo)` / `kfifo_out_peek()` | 只看不取 |
| 记录模式 | `kfifo_in_rec` / `kfifo_out_rec` | 变长记录（存时记长度） |

| 历史包袱 | 说明 |
|----------|------|
| LKD3rd 的 `kfifo_put/get` | 老 API——接口返回值语义（bool vs 字节数）与新 API 不同，读旧代码注意 |
| "必须有锁" | LKD3rd 原文按"需要上锁"教学；实际 SPSC 场景下无锁是它最大的卖点（书写作时无锁用法尚未普及） |

**HFT 对照：** 用户态网关常用 **无锁 SPSC/MPSC 环**；`kfifo` 是内核侧 **同款语义** 的官方实现（锁/关中断由调用方保证）。MPMC 场景 rte_ring 有批量化 CAS 模式，内核侧对应 `ptr_ring`（skbuff 收发路径）——**先 SPSC 后通用**是两条线共同的工程路径。

→ **Ch 8** 下半部与工作队列



<details>
<summary>自测题（点击展开）</summary>

**Q1.** kfifo 为什么适合单生产者单消费者场景？

<details><summary>答案</summary>

kfifo 是环形缓冲区，单生产者单消费者时不需要锁：生产者只写 head 指针，消费者只读 tail 指针，两者通过 unsigned 溢出回绕天然同步。这就是 DPDK rte_ring 的原理。HFT 用 kfifo/ring buffer 在网卡收包线程和交易策略线程之间传递行情数据，零锁开销。

</details>

**Q2.** kfifo 的环形缓冲区大小为什么必须是 2 的幂？

<details><summary>答案</summary>

因为环形缓冲区回绕用 `index & (size - 1)` 而非 `index % size`。位与比取模快 10x+（取模需要除法指令）。且 size 为 2^n 时 size-1 的二进制全是 1，位与等价于取模。这是内核中常见的性能优化技巧。

</details>

**Q3.** "kfifo 是无锁的"这句话哪里不严谨？

<details><summary>答案</summary>

三层限定：① 只对 **SPSC**（单生产者单消费者）成立——head/tail 各自单写者才无 lost update，MPSC/SPMC 仍需锁或 CAS；② **无锁 ≠ 无内存序**——kfifo 自身不插屏障，生产者写数据与更新 head 之间必须 `smp_wmb()`，否则消费者可能先看到新 head 再看到旧数据（弱序架构上必炸，x86 强序是运气好）；③ 中断与进程共享同一侧（如都做生产者）时退化为"关中断保护"，不是无锁。

</details>

</details>
---
