## 1. 本章定位

> **ULK Ch 2 Memory Addressing** · 80x86 内存寻址 + Linux 的软件模型

---

### 一、本章讲什么

- **硬件侧：** 80x86 上逻辑地址 → 线性地址 → 物理地址 怎么转（分段、分页、MMU、TLB）
- **软件侧：** Linux 2.6 如何**简化分段**、用**通用四级分页**适配 32/64 位，以及启动时物理内存怎么布局

Ch 1 只提到「虚拟内存、按需分页」；**本章是地址翻译的物理基础**。

---

### 二、小节导航

| 节 | 主题 |
|----|------|
| [2](./section-2-三种内存地址.md) | 三种地址、MMU 流水线 |
| [3](./section-3-分段机制.md) | GDT/LDT、Linux 四段策略 |
| [4](./section-4-硬件分页.md) | 页目录/页表、PAE、TLB |
| [5](./section-5-Linux四级分页.md) | PGD/PUD/PMD/PT、折叠 |
| [6](./section-6-内存布局与TLB.md) | 启动布局、用户/内核空间、TLB 刷新 |

---

### 三、在 Linux 链上的位置

```
Ch 2  内存寻址（本章）— 地址怎么翻译
Ch 8  内存管理       — 物理页框怎么分配
Ch 9  进程地址空间   — 每个进程怎么有自己的页表
Ch 17 页回收         — 内存不够怎么办
07 Gorman            — VM 专著（在 Ch 8–9 之后）
```

交叉：[01 CSAPP](../../../02-computer-systems/) Ch 9 · [09 MikanOS](../../../projects/P9-os-from-scratch/mikanos/) 分页实验

### 常见陷阱

1. 把 ULK Ch2 当成现代 6.x 的权威——它基于 2.6，Linux 现在用五级页表（P4D），ULK 只讲到四级
2. 混淆「分段」和「分页」——x86-64 上分段基本被禁用（flat model），Linux 2.6.20 起就 `__KERNEL_DS = __USER_DS`，分段机制只在 32 位有实际意义
3. 以为 TLB 刷新是全量的——现代内核用 `flush_tlb_mm_range()` 做范围刷新，且支持 PCID (Process Context ID) 避免刷 TLB

---

<details>
<summary>自测题（点击展开）</summary>

**Q1.** ULK Ch2 讲的四级页表（PGD→PMD→PTE）在现代 x86-64 上还够用吗？

<details><summary>答案</summary>

不够。6.x 内核在 x86-64 上用五级页表：PGD→P4D→PUD→PMD→PTE。P4D 层是 4.11 引入的，为了支持 57 位虚拟地址（LA57）。ULK 只讲到 PGD→PMD→PTE 三级或 PGD→PUD→PMD→PTE 四级。

</details>

**Q2.** Linux 64 位上分段机制还有实际作用吗？

<details><summary>答案</summary>

基本没有。x86-64 强制 flat segment model，`CS/DS/SS` 的 base 都是 0，limit 都是全空间。Linux 内核中 `__KERNEL_CS` 和 `__USER_CS` 的 base/limit 相同，区别只在 DPL（权限级）。分段在 32 位时代有意义，64 位已被分页完全取代。

</details>

**Q3.** HFT 场景下，TLB 刷新为什么是性能杀手？

<details><summary>答案</summary>

TLB miss 会触发硬件 page table walk（4-5 次内存访问）。HFT 热路径应避免 `mmap`/`mprotect` 操作（会触发 TLB shootdown），用大页（2MB/1GB）减少 TLB 条目数，绑核避免 context switch 导致的 TLB 刷新。

</details>

</details>

---

← [Ch 2 导读](../README.md) · 下一节 [2. 三种内存地址](./section-2-三种内存地址.md)
