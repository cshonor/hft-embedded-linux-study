# Day 16 · 多任务（2）


> **原书第十六章** · **调度内核升级** — **`TASKCTL`**、**sleep/wake**、4 任务并发、**LEVEL 优先级** · UI **瞬间抢占**。

---

### 本节四段结构

| 段 | 做什么 | 带走什么 |
|----|--------|----------|
| **① 任务管理自动化** | **`mtask.c` / `TASKCTL`** | **`task_alloc` / `task_run`** |
| **② 休眠唤醒** | **`task_sleep`** · FIFO wake | 空闲 **不占时间片** |
| **③ 多窗口任务** | **B0/B1/B2** | **4 任务** 并发验证 |
| **④ LEVEL 调度** | **优先级 + 层级** | 高层全睡才跑低层 · **A 抢 CPU** |

---

## 小节笔记

| 段 | 笔记 |
|----|------|
| 任务管理自动化 | [notes/section-16.1-任务管理自动化.md](./notes/section-16.1-任务管理自动化.md) |
| 休眠与唤醒 | [notes/section-16.2-休眠与唤醒.md](./notes/section-16.2-休眠与唤醒.md) |
| 增加任务窗口 | [notes/section-16.3-增加任务窗口.md](./notes/section-16.3-增加任务窗口.md) |
| 任务层级 LEVEL | [notes/section-16.4-任务层级-LEVEL.md](./notes/section-16.4-任务层级-LEVEL.md) |

---

## 本日小结

| 问题 | 答案 |
|------|------|
| 如何加任务？ | **`task_alloc` + `task_run`** · **`TASKCTL`** |
| 空闲 A 为何浪费？ | 仍轮转 → **`task_sleep`** |
| 何时醒 A？ | **FIFO 有键鼠数据 → wake** |
| 几个任务 demo？ | **A + B0/B1/B2 = 4** |
| LEVEL 规则？ | **只跑最高层未睡任务** |
| 用户体验？ | **不动鼠标 B 全速；一动 A 抢占** |

---

---

## 本日学习目标 · 自检

- [ ] 说清 **`TASKCTL` / task_alloc / task_run`**
- [ ] 描述 **sleep → wake** 与 **统一 FIFO** 的衔接
- [ ] 解释 **LEVEL 调度** vs Day 15 **平权轮转**
- [ ] 能举例 **A@L1、B@L2** 的抢占场景
- [ ] 联想到 **real-time UI vs batch** 优先级设计

---

← [Day 15](./day-15-多任务1.md) · [08-1 导读](../README.md) · [Day 17](./day-17-命令行窗口.md)

---

## 相关

- 上一日：[../day-15-multitask1/](../day-15-multitask1/)
- 下一日：[../day-17-console/](../day-17-console/)
- 模块导读：[../../README.md](../../README.md) · [../../OUTLINE.md](../../OUTLINE.md)
