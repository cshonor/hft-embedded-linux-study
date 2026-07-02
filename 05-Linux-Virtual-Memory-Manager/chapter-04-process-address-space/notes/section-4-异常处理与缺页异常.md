# Ch 4 §4 异常处理与缺页异常 (Page Faulting)

> **Understanding the Linux Virtual Memory Manager** · Mel Gorman · **选读 🟡**

### 4. 异常处理与缺页异常 (Page Faulting)

用户页 **未必常驻内存**。CPU 访问 **无有效 PTE** 或 **权限不符** 的地址 → **异常** → 内核 **`do_page_fault()`** → 架构无关 **`handle_mm_fault()`**。

### 合法缺页的三类核心场景

#### (1) 按需分配 (Demand Allocation) — **首次 touch**

| 页类型 | 读 | 写 |
|--------|----|----|
| **匿名页** | 可映射 **全局零页 (zero page)** — 读共享全 0 | **分配新物理页**，内容置 0（**COW 零页优化** 等后续演进） |
| **文件 / 设备映射** | 调 **`address_space` / 驱动** 从 **磁盘/后端** 读入页 | 可能先 **COW** 或 **分配 + 读盘** |

**HFT：** 冷路径 **prefault**（写遍每页） vs 生产 **首包 fault** — 必须选一种策略。

#### (2) 请求调页 (Demand Paging) — **从 swap 换回**

- PTE **not present**，但 **保存 swap 槽位号**
- fault 路径 **读 swap** → 填回物理页 → 更新 PTE present

**HFT：** **`mlock`** / **`swapoff`** / 足够 **RAM** — 避免热路径掉进 swap fault。

#### (3) 写时复制 (Copy-On-Write · COW) — **`fork()` 后写入**

| 步骤 | 行为 |
|------|------|
| **`fork()`** | 父子 **共享物理页**，PTE 标 **只读** |
| **一方写入** | **缺页** → 内核 **复制** 物理页 → 写者获 **私有副本**，打破共享 |

**HFT：** 多进程 **读共享** 订单簿快照可用；**写共享** 需 **`MAP_PRIVATE` COW** 或 **真共享 + 同步** — fork 后写大结构会 **突发 COW fault**。

### 缺页处理链（简图）

```
用户态访问 VA
    → MMU 异常
    → do_page_fault()
    → 查 VMA 合法？
         ├─ 否 → SIGSEGV
         └─ 是 → handle_mm_fault()
                    ├─ 首次分配 (demand alloc)
                    ├─ swap in (demand paging)
                    └─ COW break
    → 返回用户态重试指令
```

→ 接 Ch 3 **PTE 位**、Ch 10 **回收换出**、Ch 11 **swap**。

---
