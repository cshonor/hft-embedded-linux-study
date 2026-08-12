# TLPI 第 07 章 — Memory Allocation

**优先级**：🔴（堆 / 延迟分配 / 与 mmap 分界）  
**前置**：[Ch6 Processes](../chapter-06-processes/notes.md)（堆 / BSS / 地址空间）  
**后置**：[Ch8 用户与组](../chapter-08-users-and-groups/notes.md) · [Ch49 Memory Mappings](../chapter-49-memory-mappings/notes.md) · [Ch24 fork / COW](../chapter-24-process-creation/notes.md)  

---

## 小节目录

- [7.1 Program Break（程序断点）](./notes/7.1-program-break.md)
- [7.2 `brk` / `sbrk`](./notes/7.2-brk-sbrk.md)
- [7.3 `malloc` / `free`](./notes/7.3-malloc-free.md)
- [7.4 `calloc` / `realloc`](./notes/7.4-calloc-realloc.md)
- [7.5 `malloc(0)`](./notes/7.5-malloc.md)
- [7.6 glibc 双路径（拓展重点）](./notes/7.6-glibc.md)
- [7.7 典型内存错误](./notes/7.7-memory.md)

---

## 章节目标


理解堆与 **program break**、`brk`/`sbrk`；掌握 `malloc`/`calloc`/`realloc`/`free`；分清用户态分配器与内核虚拟内存交互；虚拟地址 ≠ 立刻占用物理页。

---


---

## 易错清单


1. `brk`/`sbrk` =（或封装）**syscall**；`malloc`/`free` = **库函数**。  
2. 虚拟 ≠ 物理；抬 break 不立刻吃物理页。  
3. `free` 不保证还内核；别用 `sbrk(0)` 当「真实占用」仪表。  
4. 勿写 `ptr = realloc(ptr, …)` 无临时变量。  
5. `malloc` 返回值满足最大基本对齐。  
6. 书内 mmap 章是 **Ch49**，不是 Ch48。

---


---

## 章节链路


```
Ch6  地址空间（堆在哪）
  → Ch7  brk/sbrk + malloc 族 + 双路径
  → Ch49 mmap 匿名映射
  → Ch24 fork：堆随地址空间 COW
```

---


---

## 双线提示


| 路线 | |
|------|--|
| 嵌入式 | 控泄漏；大块/常驻慎用；理解为何 RSS ≠ 所有 `malloc` 之和 |
| HFT | 热路径少 `malloc`；预分配 / 池；大块与 `mmap`/大页衔接 Ch49–50 |

---


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


---

## 参考


- Kerrisk, *The Linux Programming Interface*, **Chapter 7 — Memory Allocation**  
- [OUTLINE](../OUTLINE.md) · [Ch6](../chapter-06-processes/notes.md) · [Ch49](../chapter-49-memory-mappings/notes.md)


---

## 代码示例

```c
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>

/* Ch7 内存分配 — malloc/free vs sbrk/brk。
 * malloc 内部用 sbrk/mmap 向内核要内存；
 * 程序员只管 malloc/free，不必直接调 sbrk。
 * 编译: gcc -o ch7_demo ch7_demo.c */

int main(void) {
    /* 查看当前 program break */
    void *brk1 = sbrk(0);
    printf("initial brk = %p\n", brk1);

    /* malloc 向 libc 要内存，libc 内部可能调 sbrk */
    char *p = malloc(1024 * 1024);  /* 1 MB */
    if (!p) { perror("malloc"); return 1; }
    memset(p, 'A', 1024 * 1024);

    void *brk2 = sbrk(0);
    printf("after malloc(1MB): brk = %p (delta = %ld)\n",
           brk2, (long)(brk2 - brk1));

    /* realloc 调整大小 */
    p = realloc(p, 2 * 1024 * 1024);
    printf("realloc to 2MB: %s\n", p ? "OK" : "FAIL");

    free(p);
    printf("freed\n");
    return 0;
}

```

---

## 参考

- [OUTLINE](../OUTLINE.md)
- 原始笔记：[notes.md.bak](./notes.md.bak)
