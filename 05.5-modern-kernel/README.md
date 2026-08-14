# 05.5-modern-kernel

> 定位：**现代Linux内核（5.x / 6.x）内核子系统参考资料**
> 前置：`05-linux-kernel`（ULK/LKD 2.6时代，只用来建立内核概念框架）
> 本目录存放现代内核**非内存管理**子系统的资料，弥补旧书过时实现；
> 学习完本目录材料之后，再进入 `20-linux-kernel-deep` 做源码阅读与实操实验。

## 资料来源

1. 笨叔《奔跑吧 Linux 内核》系列（入门篇 Ch7-11 + 卷2 Ch1-2，调度/同步/中断/系统调用）
2. LWN.net 深度专题文章，专门修正 ULK3 / LKD3 的过时算法、数据结构
3. Bootlin 公开培训讲义，跟随LTS内核迭代更新，附带动手实验指引

## 内部子目录

- `book-ben-shu-notes/`  笨叔书籍读书笔记（调度 / RCU / ARM64）
- `lwn-articles-summary/`  LWN文章摘要，对标ULK过时章节（中断/同步/调度/系统调用/块设备）
- `bootlin-material/`  Bootlin讲义要点 + 实验操作清单

## 学习流转顺序

1. `05-linux-kernel`：理解内核需要解决什么问题，**不要照搬旧版代码实现**
2. `05.5-modern-kernel`：学习5.x~6.x真正的现代内核实现（非MM部分）
3. `20-linux-kernel-deep`：阅读树莓派内核源码、编写内核模块、调试实验

### ⚠️ 关键警告

ULK、LKD3基于Linux2.6。**设计思想可以借鉴，但大量结构体、函数、算法已经在6.x内核被移除重构，禁止直接对照源码查找。本目录全部材料用来补齐时代差异。**

## 参考索引文件

- [ref-modern-kernel-resources.md](./ref-modern-kernel-resources.md) — ULK3 过时章节 → LWN/官方文档详细映射

---

## 与 05-linux-kernel 的衔接

| 05 (LKD3) 章节 | 05.5 对应补充 |
|----------------|--------------|
| Ch4 调度 (CFS) | [lwn/01 EEVDF](./lwn-articles-summary/01-eevdf-scheduler.md) + [lwn/02 CFS 历史](./lwn-articles-summary/02-cfs-history.md) |
| Ch9-10 同步 | [lwn/03 RCU 基础](./lwn-articles-summary/03-rcu-basics.md) + [lwn/04 RCU 进阶](./lwn-articles-summary/04-rcu-advanced.md) + [lwn/05 qspinlock](./lwn-articles-summary/05-queued-spinlock.md) |
| Ch7-8 中断 | [lwn/06 IRQ domain](./lwn-articles-summary/06-irq-domain.md) + [lwn/07 Threaded IRQ](./lwn-articles-summary/07-threaded-irq.md) |
| Ch14 块 I/O | [lwn/08 blk-mq](./lwn-articles-summary/08-blk-mq.md) + [lwn/09 io_uring](./lwn-articles-summary/09-io-uring.md) |
| Ch5 系统调用 | [lwn/10 vDSO](./lwn-articles-summary/10-vdso.md) |
| Ch18 调试 | [lwn/11 现代调试工具](./lwn-articles-summary/11-modern-kernel-debugging.md) — eBPF/ftrace/drgn/crash |

> **学习路径：** 05 建立 2.6 时代概念框架 → 05.5 补齐 5.x/6.x 现代实现差异 → [20-linux-kernel-deep](../20-linux-kernel-deep/) 源码阅读与实操
