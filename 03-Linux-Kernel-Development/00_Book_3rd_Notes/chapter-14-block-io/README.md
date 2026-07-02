# Ch 14 块 I/O 层 · The Block I/O Layer

> **Linux Kernel Development 3rd** · Robert Love · **背景**

> 本章定位：**块设备** 与字符设备之分、**扇区/块**、**bio**、**请求队列**、**I/O 调度器（电梯）**。理解 **VFS → 页缓存 → 块层** 中磁盘路径；HFT **热路径不等盘**，但 **日志/replay** 仍要懂 **await / biolatency**。

---

## 本节结构

| 节 | 主题 | 带走什么 |
|----|------|----------|
| **① 块 vs 字符** | 设备分类 | 随机块 vs 字节流 |
| **② 扇区与块** | sector / block | 硬件 vs FS 逻辑单元 |
| **③ buffer_head** | 历史 | 2.6 前 I/O 容器 |
| **④ bio** | 现代 I/O 单元 | scatter-gather |
| **⑤ 请求队列** | `request_queue` | bio → request |
| **⑥ I/O 调度器** | 电梯算法 | 合并 · 排序 · CFQ/noop |

---

## 小节笔记

| 节 | 笔记 |
|----|------|
| 块设备与字符设备 | [notes/section-14.1-块设备与字符设备.md](./notes/section-14.1-块设备与字符设备.md) |
| 扇区与块 | [notes/section-14.2-扇区与块.md](./notes/section-14.2-扇区与块.md) |
| 缓冲区与缓冲区头 | [notes/section-14.3-缓冲区与缓冲区头.md](./notes/section-14.3-缓冲区与缓冲区头.md) |
| bio 结构 | [notes/section-14.4-bio-结构.md](./notes/section-14.4-bio-结构.md) |
| 请求队列 | [notes/section-14.5-请求队列.md](./notes/section-14.5-请求队列.md) |
| I/O 调度程序 | [notes/section-14.6-IO-调度程序.md](./notes/section-14.6-IO-调度程序.md) |
| 与上下章衔接 | [notes/section-14.7-与上下章衔接.md](./notes/section-14.7-与上下章衔接.md) |

---

## 本章小结

| 问题 | 答案 |
|------|------|
| 块 vs 字符？ | **随机块** vs **字节流** |
| sector vs block？ | **硬件扇区** vs **FS 逻辑块** |
| 现代 I/O 容器？ | **`struct bio`** + scatter-gather |
| 队列？ | **`request_queue`** · request 含 bio |
| 调度器干啥？ | **合并 + 排序** 减寻道 |
| SSD 用啥？ | **noop** — 无电梯寻道优化需求 |
| HFT？ | **热路径不等盘**；日志盘隔离 + **biolatency** |

---

## 本章学习目标 · 自检

- [ ] 解释 **bio** 如何表达 **内存不连续、磁盘连续** 的 I/O
- [ ] 说出 I/O 调度 **合并与排序** 的目的
- [ ] 对比 **Deadline**（读延迟）与 **CFQ**（进程公平）
- [ ] 知 **noop** 适用 **SSD/无寻道** 设备
- [ ] 画 **VFS → 页缓存 → bio → 队列** 简图
- [ ] 会用 **`biolatency`** 概念区分块延迟与 CPU 延迟

---

## 相关章节

- 上一章：[../chapter-13-vfs/](../chapter-13-vfs/)
- 下一章：[../chapter-15-process-address-space/](../chapter-15-process-address-space/)
- 全书导读：[../README.md](../README.md) · [../OUTLINE.md](../OUTLINE.md)
