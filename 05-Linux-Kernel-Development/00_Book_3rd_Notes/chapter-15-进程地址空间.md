# Ch 15 · 进程地址空间 · The Process Address Space

> **Linux Kernel Development 3rd** · Robert Love · **选读**  
> 本章定位：用户态 **虚拟地址空间** 的内核表示 — **`mm_struct`、VMA、mmap/munmap、页表/TLB**。衔接 **Ch 3 COW fork**、**Ch 12 物理页**、HFT **mmap/大页/mlock/零缺页**。

---

## 本节结构

| 节 | 主题 | 带走什么 |
|----|------|----------|
| **① 地址空间** | 平坦模型 · 内存区域 | text/data/bss/栈/mmap |
| **② mm_struct** | 内存描述符 | `mm_users` · `mm_count` |
| **③ VMA** | `vm_area_struct` | 权限 · `vm_ops` |
| **④ 链表+树** | mmap / mm_rb | 遍历 vs 查找 |
| **⑤ 操作 VMA** | `find_vma` 等 | `mmap_cache` |
| **⑥ mmap/munmap** | `do_mmap` / `do_munmap` | 合并相邻区 |
| **⑦ 页表** | PGD/PMD/PTE · TLB | VA → PA |

---

### ① 地址空间 · Address Spaces

Linux = **虚拟内存 OS** — 物理内存在进程间 **虚拟化**；每进程仿佛 **独占** 整片地址空间。

| 概念 | 说明 |
|------|------|
| **进程地址空间** | 进程可寻址的 **虚拟内存** + 有权使用的 **地址区间** |
| **平坦模型** | 统一 **32/64 位** 线性 VA 空间 |

#### 典型内存区域（不重叠）

| 区域 | 内容 |
|------|------|
| **text** | 可执行 **代码段** |
| **data** | **已初始化** 全局变量 |
| **bss** | **未初始化** 全局 — 常映射 **零页** |
| **栈** | 用户栈（向下增长） |
| **共享库** | `.so` 代码/数据 |
| **文件映射** | `mmap` 文件 |
| **匿名映射** | `malloc` 堆、匿名 `mmap` |

```
高地址
  ┌─────────────┐
  │ 栈           │
  ├─────────────┤
  │ mmap 区      │  ← 堆常 grow 向 mmap 靠拢
  ├─────────────┤
  │ 共享库       │
  ├─────────────┤
  │ bss / data  │
  ├─────────────┤
  │ text        │
低地址
```

**HFT：** 热路径 **`mmap` 环形缓冲**、**hugepage**、**mlock** — 都落在这些 **VMA** 上。

→ [01-CSAPP Ch9](../../01-CSAPP-3rd/chapter-09-virtual-memory/) · [Ch 3 COW](./chapter-03-进程管理.md)

---

### ② 内存描述符 · `mm_struct`

内核用 **`mm_struct`** 表示 **一个进程的地址空间**（及共享它的线程）。

| 字段 | 含义 |
|------|------|
| **`mm_users`** | **共享** 此地址空间的 **线程数**（`clone(CLONE_VM)`） |
| **`mm_count`** | **主引用计数** — 仅当 `mm_users` 皆退出后才递减；**归零释放** `mm_struct` |

#### 内核线程特例

| 事实 | 说明 |
|------|------|
| 内核线程 **无用户地址空间** | `task_struct->mm == NULL` |
| 被调度运行时 | **借用** 上一进程的 **页表** — 避免无谓切换开销 |

→ **Ch 4** `context_switch` 切换 `mm` · **Ch 3** 线程共享 `mm`

---

### ③ 虚拟内存区域 · VMA · `vm_area_struct`

Linux 中 **内存区域** = **VMA** — 地址空间内 **一段连续** 区间。

| 字段/属性 | 说明 |
|-----------|------|
| **`vm_start` ~ `vm_end`** | 区间边界 |
| **`vm_flags`** | **可读/可写/可执行** · **私有/共享** 等 |
| **`vm_ops`** | 操作该区域的 **函数指针表** |

| 视角 | 每个 VMA = 一个 **内存对象** — 独特属性与操作 |

```
mm_struct
  ├── VMA1: [0x400000, 0x401000)  text  R-X
  ├── VMA2: [0x600000, 0x602000)  data  RW-
  └── VMA3: [0x7f.., 0x7f..)      mmap  RW-  private
```

---

### ④ 内存区域的链表与树

同一 `mm_struct` 用 **两种结构** 管理 VMA：

| 结构 | 字段 | 用途 |
|------|------|------|
| **单向链表** | `mmap` | **遍历** 所有 VMA |
| **红黑树** | `mm_rb` | **按地址快速查找** VMA |

```
查找 addr 落在哪段？
    └── mm_rb 红黑树 O(log n) ──► vm_area_struct

扫描全部映射？
    └── mmap 链表 O(n)
```

→ **Ch 6** 红黑树 · **Ch 4** CFS 同数据结构

---

### ⑤ 操作内存区域

| 函数 | 作用 |
|------|------|
| **`find_vma(mm, addr)`** | 在 `mm_rb` 中找 **第一个 `vm_end > addr`** 的 VMA（含覆盖 addr 的区） |
| **`mmap_cache`** | 缓存上次查找 — **加速局部性重复访问** |
| **`find_vma_prev()`** | 找前一个 VMA |
| **`find_vma_intersection()`** | 找与给定区间 **相交** 的 VMA |

---

### ⑥ 创建与删除地址区间

#### 创建 · `do_mmap()`

| 路径 | 说明 |
|------|------|
| 用户态 | **`mmap()` / `mmap2()`** syscall |
| 内核 | **`do_mmap()`** 加入地址空间 |

| 优化 | 新区间与 **相邻 VMA 权限相同** → **合并** 成一段 |

#### 删除 · `do_munmap()`

| 用户态 | **`munmap()`** |
|--------|----------------|
| 内核 | **`do_munmap()`** 移除区间 |

→ **Ch 5** syscall · **Ch 16** 文件映射与页缓存

```c
/* 用户态概念 */
void *p = mmap(NULL, size, PROT_READ|PROT_WRITE,
               MAP_PRIVATE|MAP_ANONYMOUS, -1, 0);
munmap(p, size);
```

**HFT：** `MAP_SHARED` 跨进程共享行情缓冲 · `MAP_LOCKED`/`mlock` 防换出。

---

### ⑦ 页表 · Page Tables

程序使用 **虚拟地址（VA）**；CPU 访问 **物理地址（PA）** — **页表** 完成转换。

#### 三级页表（书中模型 · 32 位 / 通用描述）

| 级 | 名称 | 作用 |
|----|------|------|
| 1 | **PGD（页全局目录）** | 最高级索引 |
| 2 | **PMD（页中间目录）** | 中间级 |
| 3 | **PTE（页表项）** | 指向 **物理页** + 权限位 |

```
VA ──► [PGD] ──► [PMD] ──► [PTE] ──► 物理页帧号 (PFN)
```

> **x86-64** 常用 **四级**（+ PUD）；**稀疏** 大地址空间可省未用中间表 — 思想相同。

#### TLB · 转换后备缓冲器

| 硬件 | 缓存 **最近 VA→PA** 映射 |
|------|--------------------------|
| TLB miss | 走页表 — **更慢** |
| 切换 `mm` | 常 **刷新/切换 TLB** — 上下文切换成本之一 |

**HFT：** **大页（hugepage）** — 同样覆盖范围 **更少 TLB 项** → 更少 miss → 更稳延迟。

→ [01-CSAPP §9.6 TLB](../../01-CSAPP-3rd/chapter-09-virtual-memory/notes/section-9.6-9.7-地址翻译与Linux案例.md) · [06 Gorman](../../06-Linux-Virtual-Memory-Manager/)

---

### 从访问到缺页（概念）

```
用户读写 VA
    ▼
MMU 查 TLB / 页表
    ├─ 命中 ──► 访问 PA
    └─ 未映射 / 权限错 ──► page fault ──► 内核
                              ├─ COW（Ch 3）
                              ├─ 从页缓存/file 读入（Ch 16）
                              └─ 匿名页分配（Ch 12）
```

---

### Ch 15 小结

| 问题 | 答案 |
|------|------|
| 地址空间？ | 进程 **虚拟** 可寻址范围 + 多个 **VMA** |
| 内核怎么表示？ | **`mm_struct`** + **`vm_area_struct`** |
| 线程共享？ | 同 `mm` · `mm_users` / `mm_count` |
| VMA 怎么索引？ | **链表遍历** + **红黑树查找** |
| 用户如何改映射？ | **`mmap` / `munmap`** → `do_mmap` / `do_munmap` |
| VA→PA？ | **页表** + **TLB** |
| 内核线程？ | **`mm == NULL`** · 借用他人页表 |

---

### 检查单

- [ ] 列举地址空间中 **text/data/bss/栈/mmap** 等典型区
- [ ] 区分 **`mm_users` 与 `mm_count`**
- [ ] 解释为何 VMA 同时需要 **链表和红黑树**
- [ ] 说出 **`find_vma`** 与 **`mmap_cache`** 作用
- [ ] 画 **mmap 合并相邻 VMA** 的直觉
- [ ] HFT：对照 **大页、mlock、预 touch、零缺页** 策略

---

## 相关章节

- 上一章：[chapter-14-块IO层.md](./chapter-14-块IO层.md)
- 下一章：[chapter-16-页高速缓存和页回写.md](./chapter-16-页高速缓存和页回写.md)
- 本模块导读：[README.md](./README.md) · [OUTLINE.md](./OUTLINE.md)
