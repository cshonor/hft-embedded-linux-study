# TLPI 第 07 章 — Memory Allocation

**优先级**：🔴（堆 / 延迟分配 / 与 mmap 分界）
**前置**：[Ch6 Processes](../chapter-06-processes/README.md)（堆 / BSS / 地址空间 / 延迟分配）
**后置**：[Ch8 用户与组](../chapter-08-users-and-groups/README.md) · [Ch49 Memory Mappings](../chapter-49-memory-mappings/README.md) · [Ch24 fork / COW](../chapter-24-process-creation/README.md)

---

## 小节目录

- [7.1 在堆上分配内存：program break、malloc 族与两条路径](notes/7.1-heap-allocation.md)
- [7.2 在栈上分配：`alloca()`](notes/7.2-alloca.md)
- [7.3 本章小结](notes/7.3-summary.md)
- [7.4 练习：三道题，都能跑](notes/7.4-exercises.md)

> 四节的划分与 TLPI 原书一致（7.1 堆 / 7.2 alloca / 7.3 Summary / 7.4 Exercises）。原书 7.1 内部的子标题 —— `brk`/`sbrk`、`malloc`/`free`、`calloc`/`realloc`、`malloc(0)`、两条路径、对齐、典型错误 —— 都收在 7.1 一篇里。

---

## 章节目标

理解堆与 **program break**、`brk`/`sbrk`；掌握 `malloc`/`calloc`/`realloc`/`free`；分清用户态分配器与内核虚拟内存交互；虚拟地址 ≠ 立刻占用物理页。

---

## 易错清单

1. `brk`/`sbrk` = **syscall**（内核入口 `mm/mmap.c:178`）；`malloc`/`free` = **库函数**（glibc 的 ptmalloc）。
2. 虚拟 ≠ 物理；抬 break 不立刻吃物理页（延迟分配，见 [6.4](../chapter-06-processes/notes/6.4-virtual-memory.md)）。
3. **`free` 不保证还内核**；别用 `sbrk(0)` 当「真实占用」仪表 —— 用 `VmRSS`（[6.4](../chapter-06-processes/notes/6.4-virtual-memory.md)）。
4. 勿写 `ptr = realloc(ptr, …)`：失败时原指针丢失 → 泄漏。用临时变量接。
5. `malloc` 返回值满足最大基本对齐（x86-64 上是 **16**）；要更大对齐用 `posix_memalign` / `aligned_alloc`。
6. 书内 mmap 章是 **Ch49**，不是 Ch48（Ch48 是 SysV 共享内存）。
7. 「大块走 mmap」**不完整**：判据是 `nb >= mmap_threshold` 且 top chunk 也满足不了（`malloc/malloc.c:2564`）。见 [7.1 要点五](notes/7.1-heap-allocation.md#五两条路径sbrk-还是-mmap源码级判据)。
8. `malloc` **不清零**，但 glibc 的 tcache 会覆写用户区**头部 16 字节** —— 调试时别被前 16 字节骗了。
9. `realloc(p, 0)` 标准不统一（C17 及以前实现定义、C23 起 UB）；一律改写 `free(p); p = NULL;`。
10. `alloca` **没有失败返回值**，栈不够直接 `SIGSEGV`；且在**循环里不回收**（回收粒度是函数返回）。

---

## 章节链路

```text
Ch6  地址空间（堆在哪、延迟分配）
  → Ch7  brk/sbrk + malloc 族 + 两条路径（sbrk vs mmap）
  → Ch49 mmap 匿名映射 / 文件映射
  → Ch50 mprotect / mlock / madvise / mincore
  → Ch24 fork：堆随地址空间 COW
```

---

## 双线提示

| 路线 | |
|------|--|
| 嵌入式 | 控泄漏；大块/常驻慎用；理解为何 RSS ≠ 所有 `malloc` 之和；无 MMU 平台没有 `brk` |
| HFT | 热路径少 `malloc`；预分配 / 池；大块与 `mmap`/大页衔接 Ch49–50 |

---

## 背诵卡

| # | 要点 |
|---|------|
| 1 | program break = 堆上界；上移只加虚拟地址 |
| 2 | `sbrk(0)` 查边界；`sbrk(n)` 返回**调整前**的 break；业务禁用 `brk`/`sbrk` |
| 3 | `free` 小块常进空闲链表，break 未必降 |
| 4 | 两路分岔：`nb >= mp_.mmap_threshold`（且 top 不够）→ mmap |
| 5 | `realloc` 用临时指针再赋值 |
| 6 | `calloc` 强在**整数溢出检查**；大块 `calloc` 能白拿内核零页 |
| 7 | `malloc(0)` 在 glibc 返回非空、必须 `free` |
| 8 | `alloca` 只挪 `rsp`：快，但无错误检查、循环里不回收 |

---

## 参考

- Kerrisk, *The Linux Programming Interface*, **Chapter 7 — Memory Allocation**
- [OUTLINE](../OUTLINE.md) · [Ch6](../chapter-06-processes/README.md) · [Ch49](../chapter-49-memory-mappings/README.md)

---

## 代码示例

本章 `code/` 下有 9 个可在 Linux 上直接编译运行的程序，全部在 Compiler Explorer（gcc 13.3.0）上实测过，输出抄在对应笔记里。索引见 [`code/README.md`](code/README.md)。

一次编完 8 个（在 `code/` 目录下；ASan 那个单独编）：

```bash
for f in c7_*.c ex7_1_*.c; do gcc -O2 -Wall -Wextra -o "${f%.c}" "$f" || echo "FAIL $f"; done
```

最有代表性的三个：

```bash
gcc -O2 -Wall -Wextra -o c7_2_free_and_sbrk c7_2_free_and_sbrk.c && ./c7_2_free_and_sbrk
gcc -O2 -Wall -Wextra -o c7_3_two_paths   c7_3_two_paths.c   && ./c7_3_two_paths
gcc -O2 -Wall -Wextra -o c7_7_alloca      c7_7_alloca.c      && ./c7_7_alloca
```

**一句话结论**（都是实测，不是书上抄的）：

- `free` 掉 1000 个 1KB 块后，break 只从 `+1081344 B` 降到 `+143360 B` —— 剩的 140 KB 留在空闲链表里
- 16KB × 10 全从 `[heap]` 里切（只在 top 用尽时台阶式跳两级）；256KB × 4 每次新开匿名段，`[heap]` 纹丝不动
- `alloca(16MB)` 在 8MB 栈上被信号 11 杀死；循环里 `alloca(4096)` 每次固定下移 4112 字节，一路不回收
