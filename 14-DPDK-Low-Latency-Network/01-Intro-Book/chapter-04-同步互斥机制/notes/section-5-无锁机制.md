## 5. 无锁机制 (Lockless Mechanisms)

---

### 一、为何无锁

高并发下 **锁竞争** 的伤害有时 **超过数据拷贝与上下文切换** — DPDK 用 **无锁队列** 做核间 **高速包/对象传递**。

核心组件：**`rte_ring`** — 环形缓冲区，支持：

| 模式 | 缩写 |
|------|------|
| 单生产者 / 单消费者 | SP/SC |
| 多生产者 / 多消费者 | **MP/MC** |

→ mbuf 池、流水线 stage 间传递 → [chapter-02-mbuf与内存池.md](../chapter-02-mbuf与内存池.md)

---

### 二、多生产者入队原理（CAS）

以 **MP enqueue** 为例：

```
1. 各核将当前 prod_head / prod_tail 读到本地临时变量
2. CAS 尝试把 prod_head 从「旧值」更新为「旧值 + n」
   └─ 同一时刻仅一核 CAS 成功 → 获得 n 个槽位
   └─ 失败核 → 重读 head，重试
3. 成功核写入 ring 槽位数据
4. 按序更新 prod_tail（保证消费者可见顺序）
```

**关键：** **CAS 争用 head** 代替 **mutex**；数据写入与 tail 更新分离，保证 **无锁但有序**。

出队侧对称：**CAS 争用 cons_head**，再更新 `cons_tail`。

---

### 三、与 Ch3/Ch8 的衔接

| 章节 | 关联 |
|------|------|
| [Ch3 并行计算](../chapter-03-并行计算/) | 多核扩展 → 必须 **低争用** 队列 |
| [Ch8 流分类与多队列](../chapter-08-流分类与多队列/) | RSS 分核后，**核间 rte_ring** 转发未命中流 |
| [Ch2 Cache](../chapter-02-Cache与内存/notes/section-4-Cache一致性与无锁设计.md) | 伪共享、Cache line 对齐 ring 控制块 |

---

### 四、HFT 实践要点

- **预分配 ring 深度** — 避免运行时扩缩  
- **单生产者单消费者** 能确定时 — 用 **SP/SC** 变体，**零 CAS 争用**  
- **批量 enqueue/dequeue** — 摊薄 head/tail 更新开销  
- tail latency：观察 **CAS 重试率** — 与 [Ch3 Amdahl](../chapter-03-并行计算/notes/section-2-多核性能与可扩展性.md) 串行段同源  

---

← [4. 自旋锁](./section-4-自旋锁.md) · 下一节 [6. 小结与索引](./section-6-小结与索引.md)
