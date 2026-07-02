## 4. DPDK vhost 编程与封装

> 两种 API 层次 — **灵活** vs **易用**

---

### 一、vhost lib（底层）

**DPDK `vhost` 库：** 用户态 vhost 驱动 **完整实现**

| 特点 | 说明 |
|------|------|
| **细粒度 API** | 自管 socket、mem table、vring、kick |
| **灵活性最高** | 自定义交换、测试、非 Ethdev 拓扑 |
| **复杂度** | 需理解 §2–§3 全部机制 |

**适用：** 虚拟交换机、NFV 数据面、深度定制。

---

### 二、vhost PMD（Ethdev 抽象）

在 vhost lib 之上封装为 **标准 DPDK 网口**：

```c
rte_eth_rx_burst(vhost_port, ...);
rte_eth_tx_burst(vhost_port, ...);
```

| 特点 | 说明 |
|------|------|
| 与 **物理 PMD 同 API** | 复用 [Ch7 burst](../chapter-07-nic-performance-optimization/notes/section-3-IO性能深度优化.md) 习惯 |
| 屏蔽 socket/mem 细节 | 初始化阶段配置即可 |
| 多 VM | 多 **vhost-user socket** → 多 port |

**适用：** 快速原型、与现有 DPDK app（L3fwd 类）**拼 port**。

---

### 三、lib vs PMD 选型

| | **vhost lib** | **vhost PMD** |
|---|--------------|---------------|
| 控制 | 完全自控 | Ethdev 参数 |
| 性能天花板 | 相同数据路径 | 相同 |
| 开发成本 | 高 | 低 |
| 典型 | vhost-switch、OVS 集成 | 实验、简单网关 |

→ repo [chapter-03-PMD](../chapter-03-PMD与轮询模式.md) · [chapter-04 零拷贝](../chapter-04-零拷贝与用户态旁路.md)

---

← [3. DPDK vhost-user](./section-3-DPDK用户态vhost设计.md) · 下一节 [5. vhost-switch](./section-5-vhost-switch与实战.md)
