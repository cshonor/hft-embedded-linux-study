# Ch 2 §1 内存节点 (Nodes)

> **Understanding the Linux Virtual Memory Manager** · Mel Gorman · **精读 🔴**

### 1. 内存节点 (Nodes)

### NUMA 与 UMA

| 架构 | 含义 | 访问特性 |
|------|------|----------|
| **NUMA**（Non-Uniform Memory Access） | 大型机器上内存分 **多个 Bank/节点** | CPU 访问 **本地节点** 快，**远端节点** 慢（距离不同） |
| **UMA**（Uniform Memory Access） | 传统 PC / 小服务器 | 视为 **单一节点**，访问成本一致 |

VM 子系统把 **每个内存组** 称为一个 **节点 (Node)**。

### `pg_data_t`

内核用 **`pg_data_t`**（现代树中常与 **`struct pglist_data`** 对应）描述一个节点：

| UMA 系统 | NUMA 系统 |
|----------|-----------|
| 整片物理内存 ≈ **一个节点** | 多个 `pg_data_t`，各有 zone、页框 |
| 传统名 **`contig_page_data`** | 每 socket / 每 NUMA node 一个 |

**节点是物理内存的顶层容器** — 其下再划 Zone，再落到 `struct page`。

→ HFT：**`numactl --membind` / `mbind()`** 就是在 **指定 Node 上分配** — 跨 node 访问 = 额外延迟与带宽争用（→ [13-Systems-Performance Ch7](../13-Systems-Performance-2nd/chapter-07-memory/) · Hennessy 内存层次）。

---
