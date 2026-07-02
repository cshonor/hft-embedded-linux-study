# Ch 1 §4 阅读代码的策略 (Reading the Code)

> **Understanding the Linux Virtual Memory Manager** · Mel Gorman · **选读 🟡**

### 4. 阅读代码的策略 (Reading the Code)

### 为什么不从「初始化代码」读起

很多老手建议从 **boot / init** 读，但作者认为 **对 VM 不优**——初始化 **高度依赖架构**（`arch/x86/`、`setup_arch`…），容易陷进硬件细节。

### 作者推荐的 VM 阅读路线（由简入深）

掌握下面四块后，`mm/` 里反复出现的模式会清晰很多：

| 顺序 | 主题 | 源码入口 | 全书章节 | 为什么先读它 |
|:----:|------|----------|----------|--------------|
| **1** | **OOM（内存耗尽）管理器** | `mm/oom_kill.c` | [Ch 13](../../chapter-13-out-of-memory-management/) | 牵涉面广但逻辑相对 **温和**，是窥见 VM 一角的好入口 |
| **2** | **非连续内存分配器** | `mm/vmalloc.c` | [Ch 7](../../chapter-07-noncontiguous-memory-allocation/) | 功能 **基本封装在一个文件**，较独立 |
| **3** | **物理页分配器** | `mm/page_alloc.c` | [Ch 6](../../chapter-06-physical-page-allocation/) | 代码 **相对集中**；伙伴系统 / zone 都在这里 |
| **4** | **VMA 与进程内存区域** | `mm/mmap.c` 等 | [Ch 4](../../chapter-04-process-address-space/) | 与用户态 `mmap`、进程地址空间直接相关 |

```
推荐阅读流（本书作者 · 非 HFT 捷径）：

  oom_kill.c  →  vmalloc.c  →  page_alloc.c  →  mmap / VMA (Ch 4)
       │              │              │                    │
     Ch 13          Ch 7           Ch 6                 Ch 4
```

### 与本仓库 HFT 精读捷径的对照

[README · HFT 精读捷径](../README.md#hft-精读捷径) 按 **性能相关** 排序：

```
Ch 2 → Ch 3 (+ THP) → Ch 8 → Ch 4 → Ch 10
```

| | **Ch 1 作者路线** | **HFT 捷径** |
|---|-------------------|--------------|
| 目标 | 第一次 **读源码** 不迷路 | **NUMA / 大页 / slab / 布局** 理论依据 |
| 起点 | OOM → vmalloc → page_alloc | 物理内存描述 → 页表 → slab |
| 关系 | 补 **「打开 `mm/` 从哪 FILE 开始」** | 补 **「为什么 HFT 要 mlock / 大页 / 绑 NUMA」** |

两条路 **不矛盾**：可按 HFT 捷径读 **章节**；读 **源码** 时按上表四个 FILE 切入。

→ 用户态 API：[06-The-Linux-Programming-Interface](../06-The-Linux-Programming-Interface/) · 内核总览：[03-Linux-Kernel-Development](../03-Linux-Kernel-Development/) · [04-Understanding-Linux-Kernel](../04-Understanding-Linux-Kernel/) Ch 8–9

---
