## 2.4 虚拟内存与虚拟机


> ↔ [CSAPP §9.3 VM作为缓存](../../../02-computer-systems/chapter-09-virtual-memory/notes/section-9.3-虚拟内存作为缓存工具.md) · [CSAPP §9.6 地址翻译](../../../02-computer-systems/chapter-09-virtual-memory/notes/section-9.6-地址翻译.md) · [Harris §8.4 虚拟存储器](../../../00-digital-logic-cpu/ch08_memory/8.4_虚拟存储器.md)

### 虚拟内存（复习）

| 机制 | 作用 |
|------|------|
| **页表** | 虚拟地址 → 物理地址映射；进程隔离 |
| **TLB** | Translation Lookaside Buffer — 页表项缓存，**命中快、缺页慢** |
| **缺页异常** | 映射不存在或权限不符 → 内核处理，**代价极高** |

| HFT 视角 |
|----------|
| 热路径避免 **频繁缺页** — `mlock`/`mmap(MAP_LOCKED)`、启动时 **touch 完全部热页** |
| **透明大页 (THP)** / **显式 hugepage** — 减少 TLB miss；策略需与 [note-THP](../../../06-linux-mm/chapter-03-page-table-management/notes/note-透明大页THP.md) 一致 |
| 多进程/多策略：**各自地址空间** — 共享内存（SHM）需显式设计，注意 **cache 一致性** |

→ 深入：[06-Gorman](../../../06-linux-mm/) · [02-CSAPP Ch9](../../../02-computer-systems/chapter-09-virtual-memory/)

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


### 常见陷阱

- 热路径没 touch 全部页就上线 — 首次访问触发缺页 → **延迟尖刺**。必须在启动时 touch 全部热页 + mlock 锁定
- 依赖 THP 自动管理大页 — THP 的 **后台碎片整理/合并** 会引入不可预测的延迟尖刺；实盘应显式 hugepage 或关闭 THP
- 以为 VM 环境的延迟差异可忽略 — 嵌套虚拟化/设备虚拟化引入 **不可控抖动**；P99 延迟可能比裸金属高 2-5x

### 自测题（点击展开）

<details>
<summary>Q1. TLB miss 的代价是什么？为什么 hugepage 能减少 TLB miss？</summary>

TLB miss → 遍历多级页表（x86-64 四级 → 4 次内存访问）→ ~100ns。4KB 页：1GB 需 262144 个 TLB 项；2MB hugepage 只需 512 项 → TLB 覆盖范围扩大 512x → miss 大幅减少。

</details>

<details>
<summary>Q2. HFT 热路径上线前要做哪三件事来避免缺页？</summary>

1) `mlockall(MCL_CURRENT | MCL_FUTURE)` 锁定内存 → 防止 swap
2) 启动时 **touch 全部热页** → 触发缺页提前完成
3) 使用 **显式 hugepage**（`mmap(MAP_HUGETLB)`）→ 减少 TLB miss

</details>

<details>
<summary>Q3. SR-IOV 或设备直通对 HFT 网络有什么意义？</summary>

SR-IOV/直通让网卡 **绕过 hypervisor** → 减少虚拟化层开销和上下文切换 → 降低网络延迟抖动。虚拟化网络路径（vSwitch）会增加不可预测延迟，不适合延迟敏感生产。

</details>
---
