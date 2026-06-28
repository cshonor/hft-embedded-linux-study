## 5. DPDK Virtio 深度优化

> 前后端常跑在 **不同 CPU 核** — _virtqueue 更新 → 跨核 Cache 迁移_ 是主要敌人

---

### 一、问题：Available Ring 与 Cache  bouncing

频繁 **改写 Available Ring 表项**（指向不同 Descriptor）：

- 前端核写、后端核读 → **Cache Line 在核间来回**（[Ch2 伪共享](../chapter-02-cache-and-memory/notes/section-4-Cache一致性与无锁设计.md) 同类问题）  
- 额外 **描述符 分配/释放** 开销  

---

### 二、固定可用环表（单帧 mbuf）

**DPDK 创新：** Available Ring 表项与 Descriptor **固定映射**

```
Available[i] 永远对应 Descriptor[i]  （例如 i=0 永远指 desc 0）
```

| 效果 | 说明 |
|------|------|
| 更新时 **只动环指针** | 不改表项内容 → **减跨核 Cache 迁移** |
| 单帧 mbuf | 描述符 **预绑定**，减 alloc/free |
| 适用 | **单 buffer 单包** 发送路径 |

**前提：** 驱动设计为 **固定 slot** 复用 — 与通用内核 virtio 灵活分配不同。

---

### 三、Indirect 描述符 (`VIRTIO_F_RING_INDIRECT_DESC`)

**问题：** 正常发包常需 **≥2 描述符**（头 + 数据）；巨型帧 / 链式 mbuf 更多。

**Indirect：** 主队列 **只消耗 1 个描述符**，指向一块 **间接描述符表**：

```
Main queue:  desc[k] → indirect table { hdr, data, … , data_n }
```

| 收益 | 说明 |
|------|------|
| **主环描述符利用率↑** | 每包 1 slot |
| 链式 mbuf / 大包 | 间接表内展开 |
| 需协商 | 初始化时开启 **INDIRECT_DESC** feature |

→ 与 [Ch6 链式 mbuf](../chapter-06-pcie-packet-io/notes/section-6-Mbuf与Mempool.md) 发送路径配合

---

### 四、优化组合小结

```
大页 + 轮询 + SIMD（通用）
    +
固定 Available 映射（减 Cache 迁移）
    +
Indirect desc（减主环占用、提 TX 吞吐）
    =
DPDK net_virtio 极致路径
```

**HFT 提醒：** 上述优化 **无法** 弥补半虚拟化相对 SR-IOV 的 **固有延迟** — 仅「在必须 virtio 时尽量快」。

---

← [4. 驱动架构](./section-4-内核与DPDK驱动架构.md) · 下一节 [6. 小结与索引](./section-6-小结与索引.md)
