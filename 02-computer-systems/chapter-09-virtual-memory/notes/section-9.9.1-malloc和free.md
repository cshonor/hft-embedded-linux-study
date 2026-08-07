## 9.9.1 malloc 和 free

> **Ch9 §9.9.1** · [章导读](../README.md) · 上节 [§9.8 ←](./section-9.8-内存映射mmap.md) · 下节 [§9.9.2 →](./section-9.9.2-为何动态分配.md)

---

```c
void *malloc(size_t size);
void free(void *ptr);
void *calloc(size_t n, size_t size);
void *realloc(void *ptr, size_t size);
```

- 堆在 **program break** 之上增长 — `sbrk`/`brk`（内核）

---

### 常见陷阱
1. **malloc 返回未初始化内存，calloc 清零** — 不要假设 malloc 的内存是 0，否则引入难排查的 bug
2. **free 后指针不变，悬空引用（UAF）风险** — free 只标记块为空闲，不清零指针；建议 free 后立即置 NULL
3. **sbrk/brk 是系统调用，malloc 是库函数** — malloc 在用户态管理堆，只有堆不够时才 sbrk 向内核要更多页

### 自测题

<details>
<summary>Q1: malloc、calloc、realloc 的区别？</summary>

malloc(size) 分配未初始化内存；calloc(n, size) 分配并清零；realloc(ptr, size) 调整大小，可能移动内存（返回新指针，旧指针失效）。

</details>

<details>
<summary>Q2: free 一个指针后，指针本身的值变了吗？为什么有 UAF 风险？</summary>

不变。free 只标记该块为空闲，不清零指针。如果继续使用该指针（UAF），可能读到被重新分配的数据或触发 crash。

</details>

<details>
<summary>Q3: malloc 和 sbrk 的关系是什么？</summary>

malloc 是用户态库函数，管理堆的空闲链表。sbrk/brk 是系统调用，调整 program break。malloc 只在现有堆不够时才调 sbrk 向内核申请新页。

</details>

<details>
<summary>Q4: HFT 中为什么避免在热路径调用 malloc？</summary>

malloc 耗时不确定（可能触发 sbrk 系统调用 + 页表操作），且可能引起碎片。HFT 用预分配池/对象池，热路径只从池中取，零系统调用。

</details>

---

← [§9.8 ←](./section-9.8-内存映射mmap.md) · [本章导读](../README.md) · [§9.9.2 →](./section-9.9.2-为何动态分配.md)
