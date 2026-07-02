# Ch 8 下半部和推后执行的工作 · Bottom Halves and Deferring Work

> **Linux Kernel Development 3rd** · Robert Love · **精读**

> 本章定位：把 Ch 7 **上半部** 装不下的活 **推后** — **softirq / tasklet / workqueue** 选型、`ksoftirqd`、与下半部共享数据的锁。HFT **`%soft` 飙高、NAPI、收包路径抖动** 的另一半地图。

---

## 本节结构

| 节 | 主题 | 带走什么 |
|----|------|----------|
| **① 概念** | 为何需要下半部 | 缩短关中断窗口 |
| **② 历史** | BH → task queue → 现代 | **三种机制** |
| **③ softirq** | 静态 · 可同类型多 CPU 并发 | 锁 / per-CPU |
| **④ tasklet** | 基于 softirq | **同类串行** · 驱动首选 |
| **⑤ workqueue** | 工作者线程 | **唯一可睡眠** |
| **⑥ ksoftirqd** | 防饿死用户态 | nice 19 |
| **⑦ 选型** | 决策树 | sleep? → wq |
| **⑧ 锁定** | `local_bh_*` | 与自旋锁配合 |

---

## 小节笔记

| 节 | 笔记 |
|----|------|
| 下半部概念与必要性 | [notes/section-8.1-下半部概念与必要性.md](./notes/section-8.1-下半部概念与必要性.md) |
| 下半部机制的历史与演进 | [notes/section-8.2-下半部机制的历史与演进.md](./notes/section-8.2-下半部机制的历史与演进.md) |
| 软中断 | [notes/section-8.3-软中断.md](./notes/section-8.3-软中断.md) |
| tasklet | [notes/section-8.4-tasklet.md](./notes/section-8.4-tasklet.md) |
| 工作队列 | [notes/section-8.5-工作队列.md](./notes/section-8.5-工作队列.md) |
| ksoftirqd 辅助线程 | [notes/section-8.6-ksoftirqd-辅助线程.md](./notes/section-8.6-ksoftirqd-辅助线程.md) |
| 如何选择下半部机制 | [notes/section-8.7-如何选择下半部机制.md](./notes/section-8.7-如何选择下半部机制.md) |
| 锁定与禁用下半部 | [notes/section-8.8-锁定与禁用下半部.md](./notes/section-8.8-锁定与禁用下半部.md) |

---

## 本章小结

| 问题 | 答案 |
|------|------|
| 为何有下半部？ | 上半部 **快、不能睡、常关中断** |
| 现代三种？ | **softirq · tasklet · workqueue** |
| 谁可睡眠？ | **仅 workqueue** |
| tasklet vs softirq？ | tasklet **同类串行**；softirq **同类可多 CPU 并行** |
| ksoftirqd？ | softirq 过重时 **防饿死用户态** |
| 驱动默认？ | **tasklet** |
| HFT 看什么？ | **`%soft`**、NAPI、IRQ/RPS 与策略 **是否同核** |

---

## 本章学习目标 · 自检

- [ ] 列出上半部 vs 下半部 **各能/不能** 做什么
- [ ] 解释 **同类 tasklet 不会多 CPU 并行** 的意义
- [ ] 说出 **workqueue 是唯一可阻塞下半部** 的原因（进程上下文）
- [ ] 知道 **`local_bh_disable` 不挡 workqueue**
- [ ] 画 **IRQ → tasklet/softirq → socket 唤醒 → 用户 read** 简图
- [ ] 联系排障：**softirq 不高也可能因同核 cache 冲刷伤 tail**

---

## 相关章节

- 上一章：[../chapter-07-interrupts/](../chapter-07-interrupts/)
- 下一章：[../chapter-09-kernel-sync-intro/](../chapter-09-kernel-sync-intro/)
- 全书导读：[../README.md](../README.md) · [../OUTLINE.md](../OUTLINE.md)
