# Day 12 · 定时器（1）


> **原书第十二章** · OS 有了 **时间** — **PIT / IRQ0**、**TIMERCTL** 秒表、**FIFO 超时通知**、**500 路定时器** + **`next` 优化**。

---

### 本节四段结构

| 段 | 做什么 | 带走什么 |
|----|--------|----------|
| **① PIT** | 初始化 **8254 类 PIT** · **IRQ0** | **`inthandler20`** |
| **② 计量时间** | **`TIMERCTL.count`** | 启动以来 **秒数**（如 100Hz tick） |
| **③ 超时** | 倒计时结束 → **FIFO** | **定时器数组** · **`TIMER_FLAGS_USING`** |
| **④ 算法优化** | **`next`** 最近超时时刻 | 多数 tick **只比一次** |

---

## 小节笔记

| 段 | 笔记 |
|----|------|
| 初始化 PIT | [notes/section-12.1-初始化-PIT.md](./notes/section-12.1-初始化-PIT.md) |
| 计量时间 | [notes/section-12.2-计量时间.md](./notes/section-12.2-计量时间.md) |
| 超时通知 | [notes/section-12.3-超时通知.md](./notes/section-12.3-超时通知.md) |
| 加快中断处理 | [notes/section-12.4-加快中断处理.md](./notes/section-12.4-加快中断处理.md) |

---

## 本日小结

| 问题 | 答案 |
|------|------|
| 时间从哪来？ | **PIT → IRQ0 → inthandler20** |
| 怎么知道过了几秒？ | **`TIMERCTL.count`** ÷ 频率 |
| 怎么通知应用？ | 超时 → **FIFO 写事件** |
| 多路怎么管？ | **数组 + USING 标志**（500 槽） |
| 怎么不扫 500 次？ | **`next` = 最近超时 tick** |
| 为后续铺路？ | **时间片轮转 / 多任务** 必备 tick |

---

---

## 本日学习目标 · 自检

- [ ] 说清 **PIT、IRQ0、0x20 号处理** 的关系
- [ ] 会用 **count + 频率** 换算秒
- [ ] 描述 **timeout → FIFO** 与 Day 7 的衔接
- [ ] 解释 **500 定时器 + `next` 优化** 为何必要
- [ ] 联想到 **调度 tick / 最小超时事件** 概念

---

← [Day 11](./day-11-制作窗口.md) · [01 导读](../README.md) · [Day 13](./day-13-定时器2.md)

---

## 相关

- 上一日：[../day-11-window/](../day-11-window/)
- 下一日：[../day-13-timer2/](../day-13-timer2/)
- 模块导读：[../../README.md](../../README.md) · [../../OUTLINE.md](../../OUTLINE.md)
