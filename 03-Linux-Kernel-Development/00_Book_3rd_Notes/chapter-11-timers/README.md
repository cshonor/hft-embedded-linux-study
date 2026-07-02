# Ch 11 定时器和时间管理 · Timers and Time Management

> **Linux Kernel Development 3rd** · Robert Love · **精读**

> 本章定位：**HZ / jiffies**、硬件时钟、**tick 中断**、墙上时间 **`xtime`**、**动态定时器** 与 **延迟执行**。理解 **调度 tick、时间戳、定时回调** 与 HFT **延迟测量 / 节拍开销** 的关系。

---

## 本节结构

| 节 | 主题 | 带走什么 |
|----|------|----------|
| **① HZ** | 节拍率 | 精度 vs 开销 · **Tickless** |
| **② jiffies** | 全局节拍计数 | 溢出 · `time_after` |
| **③ 硬件时钟** | RTC vs 系统定时器 | 墙上时间 vs tick |
| **④ tick 处理** | `tick_periodic` | 调度 · 负载 |
| **⑤ 墙上时间** | `xtime` | seqlock · `gettimeofday` |
| **⑥ 动态定时器** | `timer_list` | **TIMER_SOFTIRQ** |
| **⑦ 延迟执行** | busy / udelay / sleep | 上下文约束 |

---

## 小节笔记

| 节 | 笔记 |
|----|------|
| 内核时间概念与节拍率 | [notes/section-11.1-内核时间概念与节拍率.md](./notes/section-11.1-内核时间概念与节拍率.md) |
| jiffies 变量 | [notes/section-11.2-jiffies-变量.md](./notes/section-11.2-jiffies-变量.md) |
| 硬件时钟和定时器 | [notes/section-11.3-硬件时钟和定时器.md](./notes/section-11.3-硬件时钟和定时器.md) |
| 定时器中断处理程序 | [notes/section-11.4-定时器中断处理程序.md](./notes/section-11.4-定时器中断处理程序.md) |
| 实际时间 / 墙上时间 | [notes/section-11.5-实际时间-墙上时间.md](./notes/section-11.5-实际时间-墙上时间.md) |
| 动态定时器 | [notes/section-11.6-动态定时器.md](./notes/section-11.6-动态定时器.md) |
| 延迟执行 | [notes/section-11.7-延迟执行.md](./notes/section-11.7-延迟执行.md) |

---

## 本章小结

| 问题 | 答案 |
|------|------|
| HZ？ | **每秒 tick 数** — 精度 vs 开销 |
| jiffies？ | **节拍计数** — 用 **`time_after`** 防回绕 |
| RTC vs 系统定时器？ | **墙上初值** vs **周期 tick** |
| tick 做什么？ | jiffies、定时器、**`scheduler_tick`**、xtime、负载 |
| 墙上时间？ | **`xtime` + seqlock** · `gettimeofday` |
| 动态定时器？ | **`timer_list`** · 一次 · **TIMER_SOFTIRQ** |
| 怎么延迟？ | **忙等 jiffies** / **`udelay`** / **`schedule_timeout`（可睡）** |

---

## 本章学习目标 · 自检

- [ ] 解释 **提高 HZ** 的利弊
- [ ] 为何比较时间用 **`time_after`** 而非裸比较
- [ ] 列出 **`tick_periodic`** 几项核心工作
- [ ] 区分 **动态定时器回调** 与 **`schedule_timeout` 睡眠** 的上下文
- [ ] 知道 **`del_timer_sync`** 为何需要「同步」
- [ ] HFT：区分 **CLOCK_MONOTONIC 测延迟** vs **墙上时间对时**

---

## 相关章节

- 上一章：[../chapter-10-sync-methods/](../chapter-10-sync-methods/)
- 下一章：[../chapter-12-memory-management/](../chapter-12-memory-management/)
- 全书导读：[../README.md](../README.md) · [../OUTLINE.md](../OUTLINE.md)
