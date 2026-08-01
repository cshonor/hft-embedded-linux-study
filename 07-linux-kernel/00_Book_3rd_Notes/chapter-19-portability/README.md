# Ch 19 可移植性 · Portability

> **Linux Kernel Development 3rd** · Robert Love · **跳过**

> 本章定位：Linux 跨 **20+ 架构** — 字长、类型、对齐、**字节序**、`HZ`/`PAGE_SIZE`、屏障、**按最坏配置写代码**（SMP/抢占/HIGHMEM）。

---

## 本节结构

| 节 | 主题 | 带走什么 |
|----|------|----------|
| **① 历史与折中** | portable vs fast | 通用 C + `arch/` 优化 |
| **② 字长与模型** | LP64 / ILP32 | 勿假设 int=long |
| **③ 特定类型** | opaque · u32 | `__u32` 对用户空间 |
| **④ 对齐与填充** | padding | 网络/磁盘布局 |
| **⑤ 字节序** | BE / LE | `cpu_to_be32` |
| **⑥ 时间与页** | HZ · PAGE_SIZE | 勿硬编码 |
| **⑦ 处理器排序** | weak ordering | `rmb`/`wmb` |
| **⑧ 最坏配置** | SMP · 抢占 · HIGHMEM | 始终按全开写 |

---

## 小节笔记

| 节 | 笔记 |
|----|------|
| 可移植 OS 与 Linux 移植史 | [notes/section-19.1-可移植-OS-与-Linux-移植史.md](./notes/section-19.1-可移植-OS-与-Linux-移植史.md) |
| 字长和数据类型 | [notes/section-19.2-字长和数据类型.md](./notes/section-19.2-字长和数据类型.md) |
| 特定数据类型 | [notes/section-19.3-特定数据类型.md](./notes/section-19.3-特定数据类型.md) |
| 数据对齐和结构体填充 | [notes/section-19.4-数据对齐和结构体填充.md](./notes/section-19.4-数据对齐和结构体填充.md) |
| 字节序 | [notes/section-19.5-字节序.md](./notes/section-19.5-字节序.md) |
| 时间与页大小 | [notes/section-19.6-时间与页大小.md](./notes/section-19.6-时间与页大小.md) |
| 处理器排序 | [notes/section-19.7-处理器排序.md](./notes/section-19.7-处理器排序.md) |
| SMP、内核抢占与高端内存 | [notes/section-19.8-SMP内核抢占与高端内存.md](./notes/section-19.8-SMP内核抢占与高端内存.md) |

---

## 本章小结

| 主题 | 要点 |
|------|------|
| 架构策略 | 通用 C + **`arch/`** 快路径 |
| 字长 | **LP64 / ILP32** · 指针勿塞 `int` |
| 类型 | **`u32`/opaque** · 明示 **signed/unsigned char** |
| 对齐 | 自然对齐 · 协议用 **固定布局** |
| 字节序 | **`cpu_to_be*`** · 无假设 |
| 时间/页 | **`HZ` `PAGE_SIZE` 宏** |
| 排序 | **屏障** |
| 配置 | **默认 SMP+抢占+HIGHMEM** |

---

## 本章学习目标 · 自检

- [ ] 说出 **LP64** 下 `int`/`long`/指针宽度
- [ ] 区分 **opaque** 与 **`u32`** 使用场景
- [ ] 解释结构体 **padding** 对 on-wire 格式的风险
- [ ] 各举 **`cpu_to_be32`** 与 `msecs_to_jiffies` 用途
- [ ] 复述 **「按最坏配置写」** 三条（SMP/抢占/HIGHMEM）
- [ ] HFT：行情字段 **固定宽度 + 网络字节序**

---

## 相关章节

- 上一章：[../chapter-18-debugging/](../chapter-18-debugging/)
- 下一章：[../chapter-20-patches-community/](../chapter-20-patches-community/)
- 全书导读：[../README.md](../README.md) · [../OUTLINE.md](../OUTLINE.md)
