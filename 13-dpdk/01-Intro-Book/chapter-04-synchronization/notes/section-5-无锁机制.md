## 5. 无锁机制 (Lockless Mechanisms)

---

### 一、为何无锁

高并发下 **锁竞争** 的伤害有时 **超过数据拷贝与上下文切换** — DPDK 用 **无锁队列** 做核间 **高速包/对象传递**。

核心组件：**`rte_ring`** — 环形缓冲区，支持：

| 模式 | 缩写 | CAS 开销 | 适用场景 |
|------|------|:---:|------|
| 单生产者 / 单消费者 | SP/SC | **零** | 固定 pipeline：RX lcore → worker lcore |
| 多生产者 / 多消费者 | **MP/MC** | 有（CAS 竞争） | 多 lcore 汇入同一队列 |

 mbuf 池、流水线 stage 间传递 → [Ch6 §6 mbuf与Mempool](../../chapter-06-pcie-packet-io/notes/section-6-Mbuf与Mempool.md)

---

### 二、rte_ring 数据结构

```c
struct rte_ring {
    char name[RTE_RING_NAMESIZE];    /* 名称 */
    int flags;                        /* SP/SC 或 MP/MC */
    unsigned size;                    /* 容量（必须 2 的幂） */
    unsigned mask;                    /* size - 1，用于位与代替取模 */
    unsigned capacity;                /* 可用容量 */

    /* 生产者控制块 — 独占 cache line */
    struct {
        uint32_t head __rte_cache_aligned;  /* 生产头 */
        uint32_t tail __rte_cache_aligned;  /* 生产尾 */
    } prod;

    /* 消费者控制块 — 独占 cache line */
    struct {
        uint32_t head __rte_cache_aligned;  /* 消费头 */
        uint32_t tail __rte_cache_aligned;  /* 消费尾 */
    } cons;

    void *ring[] __rte_cache_aligned;  /* 数据槽 — 存 mbuf 指针 */
};
```

**关键设计：**
- `head` 和 `tail` 分两个 cache line — 避免 prod 和 cons 伪共享
- `size` 必须 2 的幂 — `idx & mask` 代替 `idx % size`（取模 ~20 cycle vs 位与 1 cycle）
- 数据槽存指针（8B），不存数据本身 — mbuf 指针传递，零拷贝

---

### 三、多生产者入队原理（CAS）

以 **MP enqueue** 为例：

```
步骤 1: 争用 head
   lcore A: 读 prod_head=10, prod_tail=10
   lcore B: 读 prod_head=10, prod_tail=10
   lcore A: CAS(prod_head, 10, 12)  → 成功！获得 slot 10-11
   lcore B: CAS(prod_head, 10, 12)  → 失败（已是 12）
   lcore B: 重读 prod_head=12, CAS(prod_head, 12, 14) → 成功！获得 slot 12-13

步骤 2: 写数据
   lcore A: ring[10] = mbuf_a0; ring[11] = mbuf_a1;
   lcore B: ring[12] = mbuf_b0; ring[13] = mbuf_b1;

步骤 3: 更新 tail（按序）
   lcore A: 等 prod_tail == 10（自己读到的值）→ 更新 prod_tail = 12
   lcore B: 等 prod_tail == 12 → 更新 prod_tail = 14
```

```c
/* DPDK rte_ring_mp_enqueue_bulk 简化逻辑 */
static inline unsigned
rte_ring_mp_do_enqueue(struct rte_ring *r, void *const *obj_table,
                       unsigned n, unsigned int *free_space)
{
    uint32_t prod_head, prod_next;
    uint32_t cons_tail, free_entries;

    /* 1. 争用 head — CAS 循环 */
    do {
        prod_head = r->prod.head;
        cons_tail = r->cons.tail;
        free_entries = r->mask + cons_tail - prod_head;

        if (n > free_entries) return 0;

        prod_next = prod_head + n;
    } while (rte_atomic32_cmpset(&r->prod.head, prod_head, prod_next) == 0);
    /* ↑ CAS — 失败则重试整个循环 */

    /* 2. 写数据 — 此时其他 producer 也可并行写自己的槽位 */
    ENQUEUE_PTRS(r, &r[1], prod_head, obj_table, n);

    /* 3. 等待轮到自己更新 tail */
    while (r->prod.tail != prod_head)
        rte_pause();  /* 自旋等待前驱完成 */

    r->prod.tail = prod_next;  /* 发布 — 消费者可见 */

    return n;
}
```

**关键：** **CAS 争用 head** 代替 **mutex**；数据写入与 tail 更新分离，保证 **无锁但有序**。

---

### 四、SP/SC 变体 — 零 CAS

当确定只有一个生产者和一个消费者时，`head/tail` 无需 CAS：

```c
/* SP enqueue — 无 CAS，直接更新 */
static inline unsigned
rte_ring_sp_do_enqueue(struct rte_ring *r, void *const *obj_table, unsigned n)
{
    uint32_t prod_head = r->prod.head;
    uint32_t cons_tail = r->cons.tail;
    uint32_t free_entries = r->mask + cons_tail - prod_head;

    if (n > free_entries) return 0;

    prod_next = prod_head + n;
    r->prod.head = prod_next;  /* 直接写 — 无需 CAS */

    ENQUEUE_PTRS(r, &r[1], prod_head, obj_table, n);

    r->prod.tail = prod_next;  /* 直接发布 */
    return n;
}
```

**HFT 最佳实践：** pipeline 设计为 SP/SC 链 — 每个 lcore 只从一个 ring 读、向一个 ring 写，零 CAS 争用。

---

### 五、HFT 实践要点

- **预分配 ring 深度** — 避免运行时扩缩；深度 = 2^N（必须 2 的幂）
- **单生产者单消费者** 能确定时 — 用 **SP/SC** 变体，**零 CAS 争用**
- **批量 enqueue/dequeue** — 摊薄 head/tail 更新开销
- tail latency：观察 **CAS 重试率** — 高重试率说明争用激烈，考虑重新分配队列或改 SP/SC

```c
/* HFT 典型 pipeline：RX → Parse → Strategy → TX，全部 SP/SC */
struct rte_ring *rx_to_parse, *parse_to_strategy, *strategy_to_tx;

/* 创建 SP/SC ring */
rx_to_parse = rte_ring_create("rx_parse", 4096,
    rte_socket_id(), RING_F_SP_ENQ | RING_F_SC_DEQ);

/* RX lcore */
while (1) {
    nb_rx = rte_eth_rx_burst(port, queue, bufs, BURST);
    rte_ring_sp_enqueue_bulk(rx_to_parse, (void **)bufs, nb_rx, NULL);
}

/* Parse lcore */
while (1) {
    nb = rte_ring_sc_dequeue_burst(rx_to_parse, (void **)bufs, BURST, NULL);
    for (i = 0; i < nb; i++) parse(bufs[i]);
}
```

---

### 六、与各章节的衔接

| 章节 | 关联 |
|------|------|
| [Ch3 并行计算](../../chapter-03-parallel-computing/) | 多核扩展 → 必须 **低争用** 队列 |
| [Ch8 流分类与多队列](../../chapter-08-flow-classification-multiqueue/) | RSS 分核后，**核间 rte_ring** 转发未命中流 |
| [Ch2 Cache](../../chapter-02-cache-and-memory/notes/section-4-Cache一致性与无锁设计.md) | 伪共享、Cache line 对齐 ring 控制块 |
| [14 HFT ch07 无锁](../../../../14-hft-engineering/chapter-07-lockless-data-structures-memory-layout/) | HFT 无锁 ring 实战 |

---

← [4. 自旋锁](./section-4-自旋锁.md) · 下一节 [6. 小结与索引](./section-6-小结与索引.md)
