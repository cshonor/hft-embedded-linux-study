## 3. 指令级并发 (Instruction Concurrency)

> **单核内** 并行 — 超标量 + 乱序执行

---

### 一、超标量与乱序

现代 CPU **几乎均为超标量**：

- **无依赖** 的指令可 **乱序** 执行
- **单周期** 可 **派发/完成** 多条微操作（µop）

| 微架构 | 派发宽度 (µop/cycle) | 重排序缓冲 (ROB) | Load/Store 队列 |
|--------|:---:|:---:|:---:|
| Haswell (2013) | 4 | 192 | 72/42 |
| Skylake (2015) | 4-6 | 224 | 72/56 |
| Ice Lake (2019) | 6 | 352 | 128/72 |
| Golden Cove (2021) | 6 | 512 | 128/72 |

**IPC（每周期指令数）** 可 >1 — 理论上 6-wide 发射可达 IPC 6，实际受限于数据依赖和 cache miss。

---

### 二、对 DPDK 的含义

| 友好代码 | 不友好代码 |
|----------|------------|
| **少分支**、可预测路径 | 复杂分支、指针 chasing |
| **指令独立** — Load/Store 与算术交错 | 长 **依赖链** — 后指令等前指令 |
| 配合 **prefetch**（Ch2） | 频繁 **stall** 等内存 |
| 分支预测命中率 >99% | 分支随机 → BTB 失败 → 流水线冲刷 ~15-20 cycle |

**DPDK 热路径中的分支优化：**

```c
/* ❌ 数据依赖的分支 — 不可预测 */
if (packet_type == TYPE_A) {        // 随机分布 → 50% 预测失败
    process_type_a(pkt);
} else {
    process_type_b(pkt);
}

/* ✅ 分支消除 — 无分支代码 */
static process_fn handlers[2] = { process_type_a, process_type_b };
handlers[packet_type](pkt);         // 间接调用，但无流水线冲刷

/* ✅ 编译器提示 — likely/unlikely */
if (likely(nb_rx > 0)) {            // 告诉编译器：几乎总是成立
    /* 热路径 */
}
if (unlikely(drop_count > threshold)) {  // 几乎不成立
    /* 冷路径 */
}
```

**循环展开 — 减少分支开销：**

```c
/* DPDK 常用 — 手动展开收包循环 */
/* 4x 展开：每次迭代处理 4 个描述符 */
for (int i = 0; i < nb_pkts; i += 4) {
    /* 4 个独立 Load — 可并行发射 */
    rte_prefetch0(rx_pkts[i+1]);
    rte_prefetch0(rx_pkts[i+2]);
    rte_prefetch0(rx_pkts[i+3]);
    process_packet(rx_pkts[i]);
    process_packet(rx_pkts[i+1]);
    process_packet(rx_pkts[i+2]);
    process_packet(rx_pkts[i+3]);
}
```

**热路径：** 帮助 CPU **填满流水线** — 与 [section-4 SIMD](./section-4-数据并行与SIMD.md) `rte_memcpy` 双 Load 策略一致。

 [15-computer-architecture 流水线](../../../../15-computer-architecture/chapter-03-instruction-level-parallelism/)

---

← [2. 多核扩展](./section-2-多核性能与可扩展性.md) · 下一节 [4. SIMD](./section-4-数据并行与SIMD.md)
