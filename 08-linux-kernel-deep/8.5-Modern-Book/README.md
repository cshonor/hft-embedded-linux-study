# 现代内核资料中心

> 本文件夹存放 **ULK3 过时内容的现代替代资料**，包括笨叔《奔跑吧 Linux 内核》第2版的笔记结构、
> LWN 文章映射、官方文档等。ULK3 基于 Linux 2.6（2005年），现代内核已迭代到 6.x。

---

## 一、笨叔《奔跑吧 Linux 内核》第2版

**作者**: 笨叔（陈悦） | **出版社**: 人民邮电出版社 | **基于内核**: Linux 5.x / ARM64

| 书名 | ISBN | 出版年 | 页数 | 章数 | 对应内核 |
|------|------|--------|------|------|----------|
| 入门篇（第2版） | 978-7-115-55560-1 | 2024 | 360 | 16 | 5.0 |
| 卷1: 基础架构（第2版） | 9787115549990 | 2024 | ~500 | 9 | 5.x |
| 卷2: 调试与案例分析（第2版） | 9787115552525 | 2021 | ~360 | 6+附录 | 5.0 |

> **核心优势**: 国内跟进最勤的内核书，覆盖到 5.x/6.x，且以 ARM64 为主架构。
> ULK3 和 LKD 都停在 2.6 时代，笨叔的书是目前最好的中文现代内核入门到进阶资料。

### 入门篇（16章）

| 章 | 标题 | 对应 ULK3 / LKD | 精读? |
|----|------|-----------------|-------|
| 1 | Linux 系统基础知识 | — | 跳 |
| 2 | Linux 内核基础知识 | ULK3 Ch1 / LKD Ch1-2 | 跳 |
| 3 | ARM64 架构基础知识 | ULK3 无（ULK3 是 x86） | 选读 |
| 4 | 内核编译和调试 | — | 实验 |
| 5 | 内核模块 | ULK3 附录B | 选读 |
| 6 | 简单的字符设备驱动 | — | 跳 |
| 7 | 系统调用 | ULK3 Ch10 / LKD Ch5 | **精读** |
| 8 | 进程管理 | ULK3 Ch3 / LKD Ch3 | **精读** |
| 9 | 内存管理 | ULK3 Ch8 / LKD Ch12 | **精读** |
| 10 | 同步管理 | ULK3 Ch5 / LKD Ch9-10 | **精读** |
| 11 | 中断管理 | ULK3 Ch4 / LKD Ch7-8 | **精读** |
| 12 | 调试和性能优化 | — | 选读 |
| 13 | 开源社区 | — | 跳 |
| 14 | 文件系统 | ULK3 Ch12-16 | 选读 |
| 15 | 虚拟化与云计算 | — | 跳 |
| 16 | 综合能力训练 | — | 实验 |

### 卷1: 基础架构（9章）— 核心精读

| 章 | 标题 | 对应 ULK3 | 对应 09 (Mel Gorman) | 精读? |
|----|------|-----------|----------------------|-------|
| 1 | ARM64 架构 | ULK3 无 | — | 选读 |
| 2 | ARM64 在 Linux 内核中的实现 | ULK3 无 | — | 选读 |
| 3 | 内存管理之预备知识 | ULK3 Ch2 | 09 Ch1 | **精读** |
| 4 | 物理内存与虚拟内存 | ULK3 Ch8 | 09 Ch2+6 | **精读** |
| 5 | 内存管理之高级主题 | ULK3 Ch8 进阶 | 09 Ch8+10 | **精读** |
| 6 | 内存管理之实战案例 | ULK3 无 | 09 全书 | **精读** |
| 7 | 进程管理之基础知识 | ULK3 Ch3 | — | **精读** |
| 8 | 进程管理之调度和负载均衡 | ULK3 Ch7 | — | **精读** |
| 9 | 进程管理之调试与案例分析 | ULK3 Ch3 进阶 | — | 选读 |

### 卷2: 调试与案例分析（6章+附录）

| 章 | 标题 | 对应 ULK3 / LKD | 精读? |
|----|------|-----------------|-------|
| 1 | 并发与同步 | ULK3 Ch5 / LKD Ch9-10 | **精读** |
| 2 | 中断管理 | ULK3 Ch4 / LKD Ch7-8 | **精读** |
| 3 | 内核调试与性能优化 | — | 选读 |
| 4 | 基于 x86_64 解决宕机难题 | — | 选读 |
| 5 | 基于 ARM64 解决宕机难题 | — | 选读 |
| 6 | 安全漏洞分析 | — | 选读 |
| 附录A | 使用 DS-5 调试 ARM64 Linux 内核 | — | 参考 |
| 附录B | ARM64 中的独占访问指令 | — | 参考 |
| 附录C | 图解 MESI 状态转换 | ULK3 Ch2 相关 | 参考 |
| 附录D | 高速缓存与内存屏障 | ULK3 Ch2 相关 | 参考 |

---

## 二、ULK3 过时章节 → 笨叔 + LWN 替代映射

> 详细映射见 [ref-modern-kernel-resources.md](./ref-modern-kernel-resources.md)

| ULK3 章节 | ULK3 讲的（2.6 时代） | 现代变化（6.x 时代） | 笨叔对应章节 | LWN 文章 |
|-----------|----------------------|---------------------|-------------|----------|
| Ch4 中断 | 中断控制器、IRQ 描述符 | IRQ domain 框架、MSI/MSI-X | 入门篇 Ch11 / 卷2 Ch2 | [IRQ domains](https://lwn.net/Articles/460160/) |
| Ch5 同步 | BKL、大读者锁、自旋锁 | BKL 已删除、queued spinlock、RCU | 入门篇 Ch10 / 卷2 Ch1 | [Locked down](https://lwn.net/Articles/403178/) |
| Ch7 调度 | O(1) 调度器 | CFS → EEVDF (6.6+) | 入门篇 Ch8 / 卷1 Ch8 | [EEVDF](https://lwn.net/Articles/925371/) |
| Ch8 内存 | SLAB 分配器、zone 结构 | SLUB → SLUB/SLOB 可选、folio | 入门篇 Ch9 / 卷1 Ch3-6 | [SLUB](https://lwn.net/Articles/229096/) |
| Ch9 地址空间 | VMA 红黑树、缺页处理 | maple tree (6.1+) | 卷1 Ch4 | [Maple tree](https://lwn.net/Articles/845507/) |
| Ch10 系统调用 | int 0x80 / syscall | syscall 指令、vDSO | 入门篇 Ch7 | [vDSO](https://lwn.net/Articles/615731/) |
| Ch14 块设备 | I/O 调度器、电梯算法 | blk-mq、多队列 | — | [blk-mq](https://lwn.net/Articles/552904/) |
| Ch15 页缓存 | radix tree | XArray (5.15+) | — | [XArray](https://lwn.net/Articles/745077/) |
| Ch16 文件访问 | read/write → page cache | folio API、直接 I/O | — | [Folio](https://lwn.net/Articles/849438/) |
| Ch17 页回收 | LRU 链表 | MGLRU (6.1+)、多代 LRU | — | [MGLRU](https://lwn.net/Articles/856831/) |

---

## 三、其他现代资料

| 资料 | 说明 | 链接 |
|------|------|------|
| LWN.net | 内核开发者写的技术深度文章，每次重大改动都有覆盖 | https://lwn.net/ |
| kernel.org 官方文档 | Documentation/ 目录，6.x 后大幅完善 | https://www.kernel.org/doc/html/latest/ |
| bootlin 训练材料 | 免费内核培训文档，覆盖各子系统 | https://bootlin.com/docs/ |
| KernelTeaching | 大学内核教学项目 | https://github.com/sysprog21/kernel-threads |
| Linux 内核源码 | 6.x 源码本身就是最好的文档 | https://git.kernel.org/ |

---

## 四、推荐学习路线

```
ULK3 过时章节 → 笨叔对应章节 → LWN 文章 → 内核源码

1. 调度器: ULK3 Ch7 → 笨叔卷1 Ch8 → LWN EEVDF → kernel/sched/
2. 同步:   ULK3 Ch5 → 笨叔卷2 Ch1 → LWN RCU → kernel/locking/
3. 中断:   ULK3 Ch4 → 笨叔卷2 Ch2 → LWN IRQ → kernel/irq/
4. 内存:   ULK3 Ch8 → 笨叔卷1 Ch3-6 → LWN folio → mm/
5. 地址空间: ULK3 Ch9 → 笨叔卷1 Ch4 → LWN maple tree → mm/mmap.c
6. 系统调用: ULK3 Ch10 → 笨叔入门篇 Ch7 → LWN vDSO → arch/arm64/kernel/
```

---

## 文件结构

```
8.5-Modern-Book/
├── README.md                          ← 本文件（书目结构 + 映射表）
├── ref-modern-kernel-resources.md     ← ULK3 过时章节 → LWN/官方文档详细映射
└── (笨叔书笔记将按章节添加)
```
