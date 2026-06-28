## 4. netdev-dpdk 接口类型

`netdev-dpdk` 将多种 NFVI 连接方式 **统一为 DPDK 端口**，便于 OVS 转发面复用同一套快路径逻辑。

---

### 一、三类关键接口

| 接口类型 | 说明 | 典型场景 |
|----------|------|----------|
| **物理网口（PHY PMD）** | 高性能 **向量化 PMD** 驱动物理 NIC | 南北向流量、PHY-PHY 转发 |
| **vhost-user / vhost-cuse** | 与 VM **Virtio 前端** 高速通信 | PHY-VM-PHY、多 VM 互联 |
| **dpdkr（IVSHMEM）** | 共享内存 + **环形队列** | VM 间或进程间低拷贝互通 |

---

### 二、与虚拟化篇对照

| 本书章节 | 与 netdev-dpdk 关系 |
|----------|---------------------|
| [Ch10 SR-IOV / 透传](../chapter-10-x86-io-virtualization/) | PHY PMD 可配合 **VF 透传** — 极致性能 |
| [Ch11 Virtio](../chapter-11-virtio-paravirtualization/) | vhost 接口对接 **Virtio-net 前端** |
| [Ch12 vhost-user](../chapter-12-vhost-optimization/) | OVS **netdev-dpdk** 直接使用 vhost-user 后端 |
| [Ch13 IVSHMEM](../chapter-13-dpdk-nfv/notes/section-5-VNF深度优化设计.md) | **dpdkr** 即 IVSHMEM 路径在 OVS 中的封装 |

```
         ┌────────── OVS (dpif-netdev) ──────────┐
         │                                        │
    PHY PMD                                  vhost-user
         │                                        │
    物理网卡                              VM Virtio 前端
         │                                        │
         └──────── dpdkr (IVSHMEM) ──────────────┘
                        VM / 进程
```

---

### 三、选型权衡（复习 Ch13）

| 接口 | 性能 | 热迁移 | 安全隔离 |
|------|------|--------|----------|
| **PHY PMD / SR-IOV** | 最高 | 差 | 依赖硬件 |
| **vhost-user** | 高 | 较好 | 软件隔离 |
| **dpdkr** | 高（同机） | 受限 | 共享内存域 |

NFV 部署常 **南北向 PHY PMD + 东西向 vhost-user** 组合。

---

← [3. DPDK 加速通路](./section-3-DPDK加速数据通路.md) · 下一节 [5. 性能对比](./section-5-性能对比.md)
