# Ch 4 §2 进程地址空间描述符 (`mm_struct`)

> **Understanding the Linux Virtual Memory Manager** · Mel Gorman · **选读 🟡**

### 2. 进程地址空间描述符 (`mm_struct`)

每个进程的 **完整地址空间** 由 **`mm_struct`** 统一管理：

| 要点 | 说明 |
|------|------|
| **线程共享** | **同一进程内线程** 通常 **共享一个 `mm_struct`**（`clone` 不带 `CLONE_VM`） |
| **页表根** | 指向该进程 **PGD**（Ch 3） |
| **VMA 组织** | **链表** 遍历 + **红黑树**（`mm_rb`）按地址快速查找 |
| **统计** | **RSS**（驻留集）、虚拟大小等 |
| **锁** | **`mmap_lock`**（读写锁）保护 VMA 与页表并发 |

```
task_struct ──► mm_struct ──► pgd
                  │
                  ├── VMA 链表 / mm_rb
                  └── rss, total_vm, …
```

→ Ch 1 推荐阅读路线 **第 4 步**：VMA 创建 — [`mm/mmap.c`](https://elixir.bootlin.com/linux/latest/source/mm/mmap.c)

---
