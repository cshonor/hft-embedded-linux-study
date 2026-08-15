## 5.4 分布式共享内存与基于目录的一致性

### 监听协议的扩展瓶颈

**Snooping** 依赖 **广播** — 核/芯片数增多时：

- 总线/互连 **带宽与扇出** 压力大
- 「广播风暴」限制扩展

---

### 基于目录 (Directory-Based) 协议

| 概念 | 说明 |
|------|------|
| **目录** | 每内存块记录 **状态** + **哪些节点有副本**（位向量） |
| **点对点消息** | 仅需通知 **相关** 节点，非全体广播 |
| **适用** | **DSM / NUMA**、多 socket、大规模多核 |

**流程直觉：** 读/写 miss → 查目录 → 向持有者/共享者发 **无效或数据转移** 消息。

| HFT 视角 |
|----------|
| 双路/四路 Xeon：**跨 socket** 访问共享写变量 → 目录 + 远程 hop → **延迟尖刺** |
| 设计：**per-socket 工作集**、减少跨 socket 写共享；行情核与发单核 **同 socket** |
| 与 Linux **NUMA balancing** 的拉扯 — 实盘常 **关闭自动迁移**（`kernel.numa_balancing=0`）换确定性 |

→ [16-Systems-Performance Ch6 NUMA](../../../15-systems-performance/chapter-06-cpus/)


### 常见陷阱

- 以为监听协议（Snooping）可以无限扩展 — Snooping 依赖 **广播**，核数增多 → 总线/互连带宽爆炸 → 必须切目录协议
- 跨 socket 共享写变量不做分片 — 目录协议 + 远程 hop → 延迟尖刺；应 per-socket 工作集隔离
- 不关 NUMA balancing 就上生产 — 内核自动迁移页到「热点」节点 → 运行时不可预测延迟；实盘必须关闭

### 自测题（点击展开）

<details>
<summary>Q1. 为什么 Snooping 协议不适合大规模多核？Directory 协议如何解决？</summary>

Snooping 依赖 **广播** → 核数增多时总线/互连带宽爆炸（O(N) 广播）。Directory 为每个内存块维护 **哪些节点有副本**（位向量）→ 只需 **点对点消息** 通知相关节点 → O(相关节点数) 而非 O(全体)。

</details>

<details>
<summary>Q2. 双路 Xeon 服务器上，跨 socket 访问共享写变量为什么会延迟尖刺？</summary>

跨 socket → **目录协议查找 + 远程 hop** → 经过互连（UPI/QPI）→ 延迟 2-3x 于本地访问。如果多核频繁写同一变量 → 反复跨 socket invalidate → 延迟尖刺。对策：per-socket 工作集、行情核与发单核同 socket。

</details>

<details>
<summary>Q3. HFT 实盘为什么常关闭 NUMA balancing？怎么关？</summary>

内核 NUMA balancing 自动把页迁移到「访问最频繁」的节点 → 运行时迁移 = 不可预测延迟尖刺。实盘要确定性 → 关闭。`echo 0 > /proc/sys/kernel/numa_balancing` 或启动参数 `numa_balancing=disable`。

</details>
---
