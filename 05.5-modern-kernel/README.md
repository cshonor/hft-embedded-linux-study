# 05.5-modern-kernel

> 定位：**现代Linux内核（5.x / 6.x）内核子系统参考资料**
> 前置：`05-linux-kernel`（ULK/LKD 2.6时代，只用来建立内核概念框架）
> 本目录存放现代内核**非内存管理**子系统的资料，弥补旧书过时实现；
> 学习完本目录材料之后，再进入 `16-linux-kernel-deep` 做源码阅读与实操实验。

## 资料来源

本模块整合三个来源，按主题合并到 12 个章节中：

1. **笨叔《奔跑吧 Linux 内核》** — 中文深入讲解（调度/RCU/ARM64）
2. **LWN.net 深度专题** — 英文文章，修正 ULK3/LKD3 过时算法和数据结构
3. **Bootlin 公开培训讲义** — 跟随 LTS 内核迭代更新，附带动手实验

## 目录结构

```
chapter-XX-topic/
├── README.md      ← 章导读（来源、HFT关联、小节索引）
└── notes/         ← 按知识点拆分的笔记
```

## 全书章节（12 章）

| 章 | 主题 | 来源 | 目录 |
|----|------|------|------|
| 1 | 内核架构概述 | Bootlin | [chapter-01-kernel-architecture](./chapter-01-kernel-architecture/) |
| 2 | 调度器 (CFS→EEVDF) | 笨叔+LWN+Bootlin | [chapter-02-scheduler](./chapter-02-scheduler/) |
| 3 | RCU 现代实现 | 笨叔+LWN | [chapter-03-rcu](./chapter-03-rcu/) |
| 4 | 同步原语 (qspinlock) | LWN | [chapter-04-synchronization](./chapter-04-synchronization/) |
| 5 | 中断管理 | LWN+Bootlin | [chapter-05-interrupt-management](./chapter-05-interrupt-management/) |
| 6 | ARM64 架构 | 笨叔+Bootlin | [chapter-06-arm64-architecture](./chapter-06-arm64-architecture/) |
| 7 | ARM64 启动流程 | Bootlin | [chapter-07-arm64-boot](./chapter-07-arm64-boot/) |
| 8 | 设备驱动与设备树 | Bootlin | [chapter-08-device-driver-dt](./chapter-08-device-driver-dt/) |
| 9 | Bootloader 与构建系统 | Bootlin | [chapter-09-bootloader-build](./chapter-09-bootloader-build/) |
| 10 | PREEMPT_RT 实时内核 | Bootlin | [chapter-10-preempt-rt](./chapter-10-preempt-rt/) |
| 11 | 块 I/O 与异步 I/O | LWN | [chapter-11-block-io-async](./chapter-11-block-io-async/) |
| 12 | vDSO 与现代调试 | LWN | [chapter-12-vdso-debugging](./chapter-12-vdso-debugging/) |

## 学习流转顺序

1. `05-linux-kernel`：理解内核需要解决什么问题，**不要照搬旧版代码实现**
2. `05.5-modern-kernel`：学习5.x~6.x真正的现代内核实现（非MM部分）
3. `16-linux-kernel-deep`：阅读树莓派内核源码、编写内核模块、调试实验

### ⚠️ 关键警告

ULK、LKD3基于Linux2.6。**设计思想可以借鉴，但大量结构体、函数、算法已经在6.x内核被移除重构，禁止直接对照源码查找。本目录全部材料用来补齐时代差异。**

## 参考索引文件

- [ref-modern-kernel-resources.md](./ref-modern-kernel-resources.md) — ULK3 过时章节 → LWN/官方文档详细映射

---

## 与 05-linux-kernel 的衔接

| 05 (LKD3) 章节 | 05.5 对应补充 |
|----------------|--------------|
| Ch4 调度 (CFS) | [ch2 调度器](./chapter-02-scheduler/) — EEVDF + CFS 历史 |
| Ch5-6 同步 | [ch3 RCU](./chapter-03-rcu/) + [ch4 qspinlock](./chapter-04-synchronization/) |
| Ch7-8 中断 | [ch5 中断管理](./chapter-05-interrupt-management/) — IRQ domain + threaded IRQ |
| Ch14 块 I/O | [ch11 块I/O](./chapter-11-block-io-async/) — blk-mq + io_uring |
| Ch5 系统调用 | [ch12 vDSO](./chapter-12-vdso-debugging/) — vDSO 加速 |
| Ch18 调试 | [ch12 现代调试](./chapter-12-vdso-debugging/) — eBPF/ftrace/crash |

> **学习路径：** 05 建立 2.6 时代概念框架 → 05.5 补齐 5.x/6.x 现代实现差异 → [16-linux-kernel-deep](../16-linux-kernel-deep/) 源码阅读与实操
