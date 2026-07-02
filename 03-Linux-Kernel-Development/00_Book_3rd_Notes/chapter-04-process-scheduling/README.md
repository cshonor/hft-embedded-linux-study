# Ch 4 进程调度 · Process Scheduling

> **Linux Kernel Development 3rd** · Robert Love · **精读**

> 本章定位：**谁跑、何时跑、跑多久** — 抢占式多任务、**CFS**、休眠/唤醒、内核抢占、**RT 策略** 与 **affinity** syscall。HFT **绑核 / `SCHED_FIFO` / 抖动** 的底层地图。

---

## 本节结构

| 节 | 主题 | 带走什么 |
|----|------|----------|
| **① 演进** | 抢占式多任务 | O(1) → **CFS（2.6.23）** |
| **② 策略** | I/O vs CPU 型 · 优先级 | **nice** · **RT 0–99** |
| **③ CFS** | 公平调度算法 | **`vruntime`** · **红黑树** |
| **④ 休眠唤醒** | 等待队列 | `wake_up()` |
| **⑤ 抢占与切换** | `context_switch` | **用户/内核抢占** |
| **⑥ RT 策略** | FIFO / RR | **软实时** |
| **⑦ syscall** | 调参接口 | **affinity · yield** |

---

## 小节笔记

| 节 | 笔记 |
|----|------|
| 多任务与调度器演进 | [notes/section-4.1-多任务与调度器演进.md](./notes/section-4.1-多任务与调度器演进.md) |
| 调度策略 | [notes/section-4.2-调度策略.md](./notes/section-4.2-调度策略.md) |
| Linux 调度算法 | [notes/section-4.3-Linux-调度算法.md](./notes/section-4.3-Linux-调度算法.md) |
| 休眠与唤醒 | [notes/section-4.4-休眠与唤醒.md](./notes/section-4.4-休眠与唤醒.md) |
| 抢占与上下文切换 | [notes/section-4.5-抢占与上下文切换.md](./notes/section-4.5-抢占与上下文切换.md) |
| 实时调度策略 | [notes/section-4.6-实时调度策略.md](./notes/section-4.6-实时调度策略.md) |
| 与调度相关的系统调用 | [notes/section-4.7-与调度相关的系统调用.md](./notes/section-4.7-与调度相关的系统调用.md) |

---

## 本章小结

| 问题 | 答案 |
|------|------|
| Linux 多任务？ | **抢占式** — 调度器可强制切换 |
| 默认调度器？ | **CFS** — 公平 **比例**，非固定片长 |
| CFS 选谁？ | **`vruntime` 最小** — **红黑树最左** |
| nice vs RT？ | nice 调 **份额**；RT **0–99 压过** 所有普通进程 |
| 睡眠去哪？ | **等待队列** — 唤醒后回红黑树 |
| 内核能抢占吗？ | **能**（2.6+）— 持锁等 **非抢占** 区除外 |
| RT 策略？ | **`SCHED_FIFO` / `SCHED_RR`** — **软实时** |
| HFT 三板斧？ | **`affinity` + `chrt` + 隔离核** |

---

## 本章学习目标 · 自检

- [ ] 解释 **CFS 为何不分配传统时间片**，`vruntime` 何意
- [ ] 区分 **nice** 与 **RT 优先级** 两套数值方向
- [ ] 画出 **睡眠 → wait queue → wake_up → 红黑树**
- [ ] 说出 **`sched_setaffinity` / `sched_yield`** 用途
- [ ] 知 **软实时** 边界 — 生产上如何配 **FIFO + 绑核** 且不伤系统

---

## 相关章节

- 上一章：[../chapter-03-process-management/](../chapter-03-process-management/)
- 下一章：[../chapter-05-system-calls/](../chapter-05-system-calls/)
- 全书导读：[../README.md](../README.md) · [../OUTLINE.md](../OUTLINE.md)
