## 2.4 虚拟内存与虚拟机

### 虚拟内存（复习）

| 机制 | 作用 |
|------|------|
| **页表** | 虚拟地址 → 物理地址映射；进程隔离 |
| **TLB** | Translation Lookaside Buffer — 页表项缓存，**命中快、缺页慢** |
| **缺页异常** | 映射不存在或权限不符 → 内核处理，**代价极高** |

| HFT 视角 |
|----------|
| 热路径避免 **频繁缺页** — `mlock`/`mmap(MAP_LOCKED)`、启动时 **touch 完全部热页** |
| **透明大页 (THP)** / **显式 hugepage** — 减少 TLB miss；策略需与 [note-THP](../../../05-Linux-Virtual-Memory-Manager/chapter-03-page-table-management/notes/note-透明大页THP.md) 一致 |
| 多进程/多策略：**各自地址空间** — 共享内存（SHM）需显式设计，注意 **cache 一致性** |

→ 深入：[03-Gorman](../../../05-Linux-Virtual-Memory-Manager/) · [01-CSAPP Ch9](../../../01-CSAPP-3rd/chapter-09-virtual-memory/)

---

### 虚拟机 (VMs)

云计算与数据中心使 **隔离、迁移、多租户** 成为常态。

| 组件 | 角色 |
|------|------|
| **VMM / Hypervisor** | 虚拟化 CPU、内存、I/O；客户 OS 以为独占机器 |
| **硬件辅助** | Intel **VT-x**、AMD **SVM** — 降低陷入 VMM 的开销 |
| **安全扩展** | 如 Intel **SGX** — 细粒度 enclave 隔离 |

**挑战：** 在 **未为虚拟化设计** 的 ISA 上实现 VMM 很困难（见 2.7）。

| HFT 视角 |
|----------|
| **colo 实盘极少跑在嵌套虚拟化里** — 裸机或单租户 VM 为主；虚拟化层增加 **不可忽略的抖动** |
| 云回测集群可接受 VM；**延迟敏感生产** 要测 **裸金属 vs VM** 的 P99 差 |
| SR-IOV / 设备直通 — 减少网络虚拟化开销（衔接 DPDK 路径） |

---
