## 5. vhost-switch 与实战要点

---

### 一、vhost-switch 示例

DPDK 官方 **`vhost-switch`**：用户态 **以太网交换机**

| 组件 | 角色 |
|------|------|
| **vhost-user 端口** | 连接 **多个 VM**（多 virtio 前端） |
| **物理 NIC 端口** | 上联网络 |
| **交换逻辑** | MAC/端口转发 — 类 [Ch5 转发](../chapter-05-packet-forwarding/) |

**目标：** 宿主机上 **VM ↔ VM / VM ↔ Wire** 高速转发。

---

### 二、VMDQ 硬件分类

结合 **VMDQ（Virtual Machine Device Queue）**：

- **网卡硬件** 按 MAC/VLAN 等 **分类** 到不同队列  
- 与 [Ch8 多队列/RSS](../chapter-08-flow-classification-multiqueue/notes/section-2-网卡多队列.md) 同族  

```
Wire 入向 → NIC VMDQ 分队列 → 不同 lcore
VM 入向  → vhost-user → 交换 → NIC / 其他 VM
```

**控制面 / 数据面分核：** 管理 socket、CLI 与 **poll 转发** 分离 — [Ch5 RTC/Pipeline](../chapter-05-packet-forwarding/notes/section-3-转发框架模型.md) · [Ch7 isolcpus](../chapter-07-nic-performance-optimization/notes/section-4-平台优化与配置调优.md)

---

### 三、前后端协同清单

| 前端 [Ch11](../chapter-11-virtio-paravirtualization/) | 后端（本章） |
|------------------------------------------------------|--------------|
| Indirect desc、固定 Available | mem_table **mmap**、大页对齐 |
| Guest poll / 少 notify | Host **vhost poll** |
| 特性协商一致 | `VHOST_SET_FEATURES` 匹配 |
| mbuf 布局 | vhost 直接读 Guest 缓冲 — **布局兼容** |

**结论：** 虚拟化网络 **性能 = f(前端, 后端, 内存, 分核)** — 单点优化有顶。

---

### 四、与 HFT 的边界

| 场景 | 建议 |
|------|------|
| 交易所共置 tick | **SR-IOV 透传** [Ch10] — 非 vhost-switch |
| 云化 NFV 网关 | vhost-user + DPDK — **本章路径** |
| 混合 | 关键 VM **VF**，其余 virtio — 运维权衡 |

---

← [4. 编程封装](./section-4-vhost编程与封装.md) · 下一节 [6. 小结与索引](./section-6-小结与索引.md)
