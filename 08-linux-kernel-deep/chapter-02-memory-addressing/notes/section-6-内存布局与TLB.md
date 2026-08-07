## 6. 物理布局 · 进程页表 · 内核页表 · TLB

---

### 一、物理内存布局（启动早期）

内核启动时利用 **BIOS** 等信息建立物理地址映射：

| 要点 | 说明 |
|------|------|
| **前若干 MB** | 保留给内核代码与数据 |
| **符号边界** | `_text` · `_etext` · `_edata` · `_end` 界定内核映像在物理 RAM 中的范围 |
| **BIOS 死角** | 避开 BIOS 占用的保留区域 |

→ 启动细节：[附录 A 系统启动](../../appendix-A-system-startup.md)

---

### 二、进程线性地址空间划分（2.6 经典 3G/1G）

| 范围 | 用途 |
|------|------|
| `0x00000000` – `0xbfffffff` | **用户空间**（3 GB）；用户态可访问 |
| `0xc0000000` – `0xffffffff` | **内核空间**（1 GB）；仅内核态可访问 |

`PAGE_OFFSET` 宏通常定义在 **3 GB 边界**（`0xc0000000`）之上 —— 同一进程页表里，**内核部分所有进程共享**（2.6 经典模型）。

→ 现代 64 位布局不同，概念类似：**用户低地址 + 内核高地址映射**。

→ 深潜：[Ch 9 进程地址空间](../../chapter-09-process-address-space.md) · [07 Gorman](../../../09-linux-mm/)

---

### 三、内核页表建立过程

1. **早期临时页表** — 刚启动、RAM 信息不全时使用  
2. **主内核页全局目录** — 掌握全部可用 RAM 后建立，服务内核永久映射

---

### 四、TLB 处理

| 问题 | Linux 做法 |
|------|------------|
| 页表改了，TLB 还缓存旧翻译 | 提供 **刷新 TLB** 的函数 |
| SMP 上切换进程开销大 | **Lazy TLB mode** — 延迟刷新，优化上下文切换 |

页表变更路径（`mmap`、`munmap`、进程切换）都会涉及 TLB shootdown —— HFT 里大页、pin 内存与 **TLB 命中率** 相关。

---

### 五、后续章节索引

| Ch 2 主题 | 继续读 |
|-----------|--------|
| 页框分配、伙伴系统 | [Ch 8 内存管理](../../chapter-08-memory-management.md) 🔴 |
| 进程 VMA、缺页、COW | [Ch 9 进程地址空间](../../chapter-09-process-address-space.md) 🔴 |
| 回收、swap | [Ch 17 页回收](../../chapter-17-page-reclaim.md) 🟡 |
| VM 专著 | [07 Gorman](../../../09-linux-mm/) |
| 用户态视角 | [01 CSAPP](../../../02-computer-systems/) Ch 9 · [08 TLPI](../../../04-linux-userspace-api/) |
| 动手分页 | [09 MikanOS](../../../05-os-from-scratch/mikanos/) |

### 常见陷阱

1. 以为内核虚拟地址空间布局和 ULK 讲的一样——6.x 内核的布局有变化（module 区移到 0xffffffffa0000000 附近，KASLR 打乱了内核代码基址）
2. 混淆 `ZONE_DMA`/`ZONE_DMA32`/`ZONE_NORMAL` 的边界——这取决于架构，x86-64 上 DMA=16MB，DMA32=4GB，其余是 NORMAL
3. 以为 TLB 刷新是即时的——TLB shootdown 需要跨 CPU IPI，是异步操作，在 HFT 场景可能导致微秒级抖动

---

<details>
<summary>自测题（点击展开）</summary>

**Q1.** x86-64 内核虚拟地址空间的三大区域是什么？

<details><summary>答案</summary>

① direct mapping（直射区）：`0xffff888000000000` 起，映射所有物理内存；② vmalloc area：`0xffffc90000000000` 起，用于 `vmalloc()`；③ vmemmap：`0xffffea0000000000` 起，`struct page` 数组。ULK 讲的布局基于 32 位，地址完全不同。

</details>

**Q2.** 为什么 HFT 要避免跨 CPU 的内存操作？

<details><summary>答案</summary>

跨 CPU 访问共享数据可能导致 TLB shootdown（IPI 中断其他 CPU 刷新 TLB），耗时数微秒。绑核 + per-CPU 数据结构可以避免这个问题。`/proc/interrupts` 中的 `TLB` 行可以观察 shootdown 频率。

</details>

**Q3.** KASLR 对内核调试有什么影响？

<details><summary>答案</summary>

KASLR（Kernel Address Space Layout Randomization）随机化内核代码加载基址，`/proc/kallsyms` 默认显示 0 地址（非 root）。调试时需要 `nokaslr` 启动参数禁用，或用 `kptr_restrict=0` 暴露真实地址。HFT 生产环境通常保留 KASLR（安全）但调试时禁用。

</details>

</details>

---

← [5. Linux 四级分页](./section-5-Linux四级分页.md) · 下一章 [Ch 3 进程](../../chapter-03-processes.md)
