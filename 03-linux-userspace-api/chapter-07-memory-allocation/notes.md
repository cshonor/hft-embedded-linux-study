# TLPI 第 07 章 — Memory Allocation

> 对应目录：`chapter-07-memory-allocation/`  
> 书名原文：**Memory Allocation**  
> ⚠️ `brk`/`sbrk` 只为理解底层；业务代码用 `malloc` 族，大块另见 [Ch49 mmap](../chapter-49-memory-mappings/notes.md)。

**优先级**：🔴（堆 / 延迟分配 / 与 mmap 分界）  
**前置**：[Ch6 Processes](../chapter-06-processes/notes.md)（堆 / BSS / 地址空间）  
**后置**：[Ch8 用户与组](../chapter-08-users-and-groups/notes.md) · [Ch49 Memory Mappings](../chapter-49-memory-mappings/notes.md) · [Ch24 fork / COW](../chapter-24-process-creation/notes.md)  
**体系对照**：CSAPP Ch9 · [LKD 地址空间](../../05-linux-kernel/00_Book_3rd_Notes/chapter-15-process-address-space/)

---

## 章节目标

理解堆与 **program break**、`brk`/`sbrk`；掌握 `malloc`/`calloc`/`realloc`/`free`；分清用户态分配器与内核虚拟内存交互；虚拟地址 ≠ 立刻占用物理页。

---

## 7.1 Program Break（程序断点）

堆在 BSS 上方；**program break** = 堆当前上边界。

| 动作 | 效果 |
|------|------|
| 上移断点 | 扩大堆，获得更多**虚拟**地址空间 |
| 下移断点 | 缩小堆，归还虚拟地址给内核 |

> **关键：** 抬高断点**只预留虚拟地址**；**首次访问**时才通过缺页异常分配物理页（延迟分配）。

---

## 7.2 `brk` / `sbrk`

```c
#include <unistd.h>
int brk(void *end_data_segment);     /* 断点设到绝对地址；0 成功，-1 失败 */
void *sbrk(intptr_t increment);      /* 相对移动；成功返回调整前地址 */
```

| 规则 | |
|------|--|
| `sbrk(0)` | **不移动**，返回当前 break（查边界） |
| `sbrk` 成功 | 返回**调整前**的 break；失败 `(void *)-1` |
| Linux | `sbrk` 多是 glibc 封装，底层 `brk` |
| 业务代码 | **禁止**直接 `brk`/`sbrk`；只为理解 `malloc` |
| 下移过猛 | 勿把断点压到初始堆底以下 → 易 `SIGSEGV` |

Demo：[`code/sbrk_probe.c`](./code/sbrk_probe.c)

---

## 7.3 `malloc` / `free`

```c
#include <stdlib.h>
void *malloc(size_t size);   /* 未初始化；失败 NULL + errno */
void free(void *ptr);
```

### 块布局

用户指针**不含**分配器头部；元数据（大小、链表指针等）在用户区**前方**。  
越界写 → 破坏头部 → `free` 常直接崩。

### `free` 要点

1. `ptr` 必须来自 `malloc`/`calloc`/`realloc`；否则 UB。  
2. `free(NULL)` 安全、空操作。  
3. **`free` 不一定下调 program break**  
   - 小块：进 glibc 空闲链表，供后续复用（少 syscall）  
   - 大块连续空闲：才可能 `sbrk(-n)` 还虚拟内存  
4. `free` **不清零**内容（信息残留风险）。

Demo：[`code/free_and_sbrk.c`](./code/free_and_sbrk.c)（Listing 7-1 精神）

---

## 7.4 `calloc` / `realloc`

```c
void *calloc(size_t numitems, size_t size);  /* 置零 */
void *realloc(void *ptr, size_t newsize);
```

| | |
|--|--|
| `calloc(n,s)` | ≈ `malloc(n*s)` + 清零；适合数组 |
| `realloc` 扩容 | 后方够 → 原地；不够 → 新块 + 拷贝 + 释放旧块（指针可能变） |
| `realloc` 缩容 | 截断尾部；起始指针通常不变 |

```c
/* ❌ ptr = realloc(ptr, n); 失败时原指针丢失 → 泄漏 */
void *np = realloc(ptr, newsize);
if (np == NULL) { /* ptr 仍有效 */ }
else { ptr = np; }
```

另：`realloc(NULL, n)` ≈ `malloc(n)`；`realloc(ptr, 0)` 平台不统一，**尽量避免**。

---

## 7.5 `malloc(0)`

POSIX 允许返回 `NULL` **或** 可 `free` 的非空指针。  
**glibc：常返回有效指针，必须 `free`**，不能当「失败」处理。

---

## 7.6 glibc 双路径（拓展重点）

| 路径 | 方式 | 特点 |
|------|------|------|
| 小块 | `sbrk` 扩主堆 | 释放后常留在空闲链表 |
| 大块（默认约 >128KB，可配） | `mmap(MAP_ANONYMOUS)` | `free` 时常**立刻还内核**；不绑在 program break 上 |

→ 另一种拿虚拟内存的主路径：书内 [Ch49 Memory Mappings](../chapter-49-memory-mappings/notes.md)（勿与 SysV SHM Ch48 混淆）。

---

## 7.7 典型内存错误

1. **泄漏**：丢指针未 `free`  
2. **双重 free**：破坏空闲链表  
3. **野指针 free**：栈/全局/非堆指针  
4. **越界**：覆盖块头  
5. **悬挂指针**：`free` 后继续用  

---

## 易错清单

1. `brk`/`sbrk` =（或封装）**syscall**；`malloc`/`free` = **库函数**。  
2. 虚拟 ≠ 物理；抬 break 不立刻吃物理页。  
3. `free` 不保证还内核；别用 `sbrk(0)` 当「真实占用」仪表。  
4. 勿写 `ptr = realloc(ptr, …)` 无临时变量。  
5. `malloc` 返回值满足最大基本对齐。  
6. 书内 mmap 章是 **Ch49**，不是 Ch48。

---

## 章节链路

```
Ch6  地址空间（堆在哪）
  → Ch7  brk/sbrk + malloc 族 + 双路径
  → Ch49 mmap 匿名映射
  → Ch24 fork：堆随地址空间 COW
```

---

## 双线提示

| 路线 | |
|------|--|
| 嵌入式 | 控泄漏；大块/常驻慎用；理解为何 RSS ≠ 所有 `malloc` 之和 |
| HFT | 热路径少 `malloc`；预分配 / 池；大块与 `mmap`/大页衔接 Ch49–50 |

---

## 背诵卡

| # | 要点 |
|---|------|
| 1 | program break = 堆上界；上移只加虚拟地址 |
| 2 | `sbrk(0)` 查边界；业务禁用 `brk`/`sbrk` |
| 3 | `free` 小块常进空闲链表，break 未必降 |
| 4 | 小块 sbrk / 大块 mmap |
| 5 | `realloc` 用临时指针再赋值 |

---

## 参考

- Kerrisk, *The Linux Programming Interface*, **Chapter 7 — Memory Allocation**  
- [OUTLINE](../OUTLINE.md) · [Ch6](../chapter-06-processes/notes.md) · [Ch49](../chapter-49-memory-mappings/notes.md)
