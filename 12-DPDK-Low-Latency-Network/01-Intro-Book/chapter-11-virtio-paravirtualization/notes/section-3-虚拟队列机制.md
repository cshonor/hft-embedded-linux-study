## 3. 虚拟队列 (Virtqueue) 机制

> 连接 **前端驱动** 与 **后端驱动** 的实际数据链路

---

### 一、为何需要三环设计

传统双指针环：**描述符只能顺序执行** — 中间条目被后端占用时，前端 **难以回收** 后续已完成项。

Virtio **三张表** — 「无障碍」提交与回收：

---

### 二、三张表

| 表 | 生产者 | 消费者 | 作用 |
|----|--------|--------|------|
| **Descriptor Table** | 前端分配 | 双方读 | 指向 Guest 内存 **数据缓冲区** |
| **Available Ring** | **前端写** | 后端读 | 「这些描述符 **可用了**」 |
| **Used Ring** | **后端写** | 前端读 | 「这些描述符 **我用完了**」 |

```
前端：填 Descriptor → 挂到 Available Ring → kick 后端
后端：从 Available 取 desc → 处理 → 写入 Used Ring → 通知前端
前端：从 Used Ring 回收 desc
```

**关键：** 即使 **中间** 描述符仍被后端占用，前端仍可从 Used Ring **回收其他已完成** 描述符 — 避免 head-of-line blocking。

---

### 三、与 DPDK 描述符环对照

| | Virtio Virtqueue | [Ch6 NIC DMA 环](../chapter-06-pcie-packet-io/notes/section-3-DMA描述符环形队列.md) |
|---|------------------|-------------------------------------------------------------------------------------|
| 位置 | Guest 内存，后端 **共享访问** | 设备 DMA 环 |
| 同步 | Available/Used **分离** | Head/Tail + DD 位 |
| 跨核 | 前后端常 **不同 CPU** — Cache 问题（§5） | 单端 PMD 轮询为主 |

---

### 四、Kick / Notify

- 前端更新 Available 后 **notify** 后端（或依赖 **轮询** 减中断）  
- DPDK virtio PMD 倾向 **poll** — 与 [Ch7 轮询模式](../chapter-07-nic-performance-optimization/notes/section-2-轮询与混合中断模式.md) 一致

---

← [2. Virtio 规范](./section-2-Virtio规范与使用场景.md) · 下一节 [4. 驱动架构](./section-4-内核与DPDK驱动架构.md)
