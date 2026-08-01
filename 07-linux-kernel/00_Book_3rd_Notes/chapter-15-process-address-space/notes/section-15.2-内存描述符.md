## ② 内存描述符 · `mm_struct`

**一个进程地址空间** 在内核里主要由 **`mm_struct`** 描述 — **同一地址空间的所有线程** 共享同一 **`mm`** 指针。

#### 关键字段

| 字段 | 含义 |
|------|------|
| **`mmap` / `mm_rb`** | VMA **链表** + **红黑树**（Ch 15.4） |
| **`pgd`** | **页全局目录** — 此空间的 **页表根** |
| **`mm_users`** | **地址空间用户数** — 多少 **线程** 指向此 `mm`（`CLONE_VM`） |
| **`mm_count`** | **`mm_struct` 本身引用计数** — **内核持有者**（如 core dump、`/proc`） |
| **`total_vm` / `locked_vm`** | 映射页统计 — **`mlock` 计入 locked** |
| **`def_flags`** | **`mmap` 默认 flags** |

#### 引用计数语义

```
线程 A ──┐
线程 B ──┼──► mm_struct  (mm_users = 2)
线程 C ──┘

某线程 exit: mm_users--
mm_users == 0 且 mm_count == 0 → 释放全部 VMA、页表、映射
```

| 计数 | 谁 bump |
|------|---------|
| **`mm_users`** | **`fork` 共享 VM**、**线程创建** |
| **`mm_count`** | **`get_task_mm()`**、**ptrace**、**lazy TLB** 等内核引用 |

#### 内核线程特例

| 事实 | 说明 |
|------|------|
| **`task_struct->mm == NULL`** | 纯内核线程 **无用户映射** |
| 被调度运行时 | **借用** 上一用户进程的 **`active_mm`** — **延迟 TLB flush** |
| **`active_mm`** | 记录 **最近一次** 在该核运行的用户 `mm` |

#### 与调度 / 切换（Ch 4）

| 事件 | `mm` 行为 |
|------|-----------|
| **`context_switch`** | 若 `next->mm != prev->mm` → **切换页表根（CR3）** → **TLB 部分失效** |
| **同线程组切换** | 同 `mm` — **省 TLB flush** |
| **绑核 + 单进程** | 切换少 — **HFT 友好** |

#### `/proc` 与调试

| 路径 | 内容 |
|------|------|
| **`/proc/pid/maps`** | 所有 **VMA** 区间 |
| **`/proc/pid/smaps`** | 每 VMA **RSS / Pss / 大页** |
| **`/proc/pid/pagemap`** | **PFN**（需 root）— 验证 **是否 huge / 是否 swap** |

**HFT：** 上线前 **`smaps` 确认 locked**、**`pagemap` 确认 2MB huge**；**意外 `mm_users` 泄漏** 少见，但 **子进程 `fork` 复制地址空间** 会 **COW 尖刺** — 热路径 **posix_spawn / vfork 策略** 或 **启动后不再 fork**。

→ [Ch 4 context_switch](../../chapter-04-process-scheduling/notes/section-4.5-抢占与上下文切换.md) · [Ch 3 线程共享 mm](../../chapter-03-process-management/) · [06 Gorman mm_struct](../../../../09-linux-mm/chapter-04-process-address-space/)

---
