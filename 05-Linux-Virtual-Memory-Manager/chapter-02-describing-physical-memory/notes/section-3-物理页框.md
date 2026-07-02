# Ch 2 §3 物理页框 (Pages)

> **Understanding the Linux Virtual Memory Manager** · Mel Gorman · **精读 🔴**

### 3. 物理页框 (Pages)

系统中 **每个物理页框** 对应一个 **`struct page`** — 内核用它 **持续跟踪** 该页框状态（无论是否已映射到用户空间）。

### 核心字段（原书强调）

| 字段 | 作用 |
|------|------|
| **`mapping`** | 若页属于 **文件映射**，指向该文件的 **address space**；匿名页 / slab 等另有用法 |
| **`count`** | **引用计数**；`0` → 可释放；`>0` → 正被进程或内核使用 |
| **`flags`** | 描述页当前状态的 **标志位集合** |

### 页面状态标志 (Page Flags) — 示例

| 标志 | 含义 |
|------|------|
| **`PG_active`** | **热页** — 在 **active LRU** 链上，近期被访问 |
| **`PG_dirty`** | 页内容已改，需 **写回** 磁盘 |
| **`PG_locked`** | 正被 **I/O 锁定** |
| **`PG_uptodate`** | 已从磁盘 **成功读入**，内容有效 |

LRU、dirty、locked 等标志与 **Ch 10 页框回收**、**swap** 直接相连 — 回收器决定 **踢哪一页**，就是看 **哪个 zone 的哪条 LRU、什么 flags**。

→ 用户态 `mmap` 的文件页、匿名页，最终都落实为 **`struct page` + 引用计数 + flags**（→ [06-The-Linux-Programming-Interface](../06-The-Linux-Programming-Interface/) · [03-Linux-Kernel-Development Ch12](../03-Linux-Kernel-Development/00_Book_3rd_Notes/chapter-12-memory-management/)）。

---
