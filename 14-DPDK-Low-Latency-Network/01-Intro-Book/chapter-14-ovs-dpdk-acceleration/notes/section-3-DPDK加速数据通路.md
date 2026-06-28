## 3. DPDK 加速的 OVS 数据通路

> **OVS 2.4+** 引入 DPDK 支持 — 核心思想：**数据通路完全用户态**

---

### 一、旁路内核

| DPDK 机制 | 在 OVS 中的作用 |
|-----------|-----------------|
| **PMD 轮询** | 物理网口报文 **不经 Linux 内核** 直接进入用户态 OVS |
| **向量指令** | 批量处理、提高每周期报文数 |
| **大页内存** | 减少 TLB miss，稳定转发延迟 |
| **绑核（isolcpus）** | 转发线程独占核，避免调度抖动 |

```
物理网口 ──PMD──► 用户态 OVS（dpif-netdev）
                      │
                      ├──► 物理网口 / vhost / dpdkr
                      │
                      └── 不经过 openvswitch.ko 快路径
```

→ [Ch7 PMD / burst](../chapter-07-nic-performance-optimization/) · [Ch2 大页](../chapter-02-cache-and-memory/notes/section-5-大页Hugepages.md)

---

### 二、软件架构扩展

DPDK 加速 OVS 引入两个关键模块：

| 模块 | 角色 |
|------|------|
| **`dpif-netdev`** | 用户态 **快速通路** 实现 — 替代内核 `openvswitch.ko` 的数据面 |
| **`netdev-dpdk`** | 基于 DPDK 的 **网络设备抽象** — 统一 PHY、vhost、IVSHMEM 等端口 |

```
ovs-vswitchd
    ├── OpenFlow / 流表逻辑（仍可在用户态）
    └── dpif-netdev（用户态快路径）
            └── netdev-dpdk（DPDK 端口抽象）
                    ├── 物理 PMD 口
                    ├── vhost-user / vhost-cuse
                    └── dpdkr（IVSHMEM）
```

**与 Ch13 呼应：** OPNFV 栈中 OpenStack + KVM + **OVS-DPDK** 是常见 NFVI 组合。

---

### 三、慢路径 vs 快路径（DPDK 版）

| 路径 | 传统 OVS | DPDK OVS |
|------|----------|----------|
| **快路径** | 内核 `openvswitch.ko` | **用户态 `dpif-netdev`** |
| **慢路径** | netlink 上送 `ovs-vswitchd` | 仍在用户态 — **减少跨空间切换** |
| **I/O** | 内核 netdevice | **DPDK PMD / vhost PMD** |

→ [Ch4 无锁 ring](../chapter-04-synchronization/notes/section-5-无锁机制.md) — OVS 内部队列与 DPDK ring 思想相通

---

← [2. 传统 OVS](./section-2-传统OVS架构与瓶颈.md) · 下一节 [4. netdev-dpdk 接口](./section-4-netdev-dpdk接口类型.md)
