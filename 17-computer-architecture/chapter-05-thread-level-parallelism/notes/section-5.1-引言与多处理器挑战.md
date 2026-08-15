## 5.1 引言与多处理器的挑战


> ↔ [CSAPP §12.6 线程并行性](../../../02-computer-systems/chapter-12-concurrent-programming/notes/section-12.6-使用线程提高并行性.md) · [Harris §7.7 高级微结构](../../../00-digital-logic-cpu/ch07_microarchitecture/7.7_高级微结构.md)

### 为何转向 TLP？

| 背景 | 结果 |
|------|------|
| **ILP 收益递减** | 单核难以再大幅提速 |
| **功耗墙 / Dennard 终结** | 多核成为主流扩展路径 |
| **MIMD** | 多指令流、多数据流 — **线程** 是自然并行单位 |

---

### 多处理器分类

| 类型 | 别名 | 特点 | 规模 |
|------|------|------|------|
| **对称共享内存 (SMP)** | 集中式共享、**UMA** | 所有核对内存 **近似均匀延迟** | 通常 ≤ 数十核 |
| **分布式共享内存 (DSM)** | **NUMA** | 内存 **分节点**；本地快、远程慢 | 大规模服务器 |

**NUMA 直觉：** 每个 socket 有 **本地 DRAM**；访问他节点内存经 **互连**，延迟与带宽均劣于本地。

---

### 并行处理的两大障碍

| 障碍 | 说明 |
|------|------|
| **有限并行性** | [Amdahl 定律](../../chapter-01-quantitative-design-fundamentals/notes/section-1.9-计算机设计的量化原则.md) — 串行段封顶加速比 |
| **通信与访存代价** | 远程内存、一致性流量、锁竞争 |

| HFT 视角 |
|----------|
| **绑核 + NUMA 本地分配** — `numactl --membind`、`libnuma`；行情 buffer 与处理线程 **同节点** |
| 策略并行度受 **串行段**（单订单簿、单连接排序）限制 — 先 profile 再找 p |
| 多 socket 机：**跨 NUMA 访问** 可吃掉微优化收益 → [16-Systems-Performance Ch6](../../../14-systems-performance/chapter-06-cpus/) |


### 常见陷阱

- 以为加核就能线性加速 — Amdahl 定律：串行段封顶加速比；订单簿单连接排序等串行段不消除，加核无益
- 忽略 NUMA 拓扑 — 跨 socket 访问共享写变量 → 目录协议 + 远程 hop → **延迟尖刺**；行情核与发单核必须同 socket
- 不关闭 NUMA balancing — 内核自动迁移页 → 运行时 **不可预测延迟**；实盘应 `numa_balancing=0` 换确定性

### 自测题（点击展开）

<details>
<summary>Q1. Amdahl 定律：程序 90% 可并行，10% 串行。无限核加速比上限是多少？</summary>

S = 1 / (0.1 + 0.9/∞) = 1 / 0.1 = **10x**。即使无限核，10% 串行段将加速比封顶在 10x。→ 先 profile 找串行段，再决定并行策略。

</details>

<details>
<summary>Q2. UMA 和 NUMA 的区别？HFT 为什么要绑核 + NUMA 本地分配？</summary>

UMA：所有核对内存均匀延迟（SMP，≤数十核）。NUMA：内存分节点，本地快远程慢（DSM）。HFT 绑核 + 本地分配 → 行情 buffer 和处理线程 **同节点** → 避免跨 socket 访问的 ~2x 延迟惩罚。

</details>

<details>
<summary>Q3. HFT 服务器上为什么要关闭 `numa_balancing`？怎么关？</summary>

内核 NUMA balancing 自动把页迁移到「访问最频繁」的节点 → 运行时迁移 = 不可预测延迟。关闭：`echo 0 > /proc/sys/kernel/numa_balancing` 或启动参数 `numa_balancing=disable`。

</details>
---
