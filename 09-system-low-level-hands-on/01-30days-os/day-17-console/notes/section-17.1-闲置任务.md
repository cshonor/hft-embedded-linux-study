## ① 闲置任务 · 创建命令行窗口

#### 为何独立 `console_task`？

命令行逻辑 **不应堆进 `HariMain`** — 与 Day 16 **`task_alloc/task_run`** 一致：

```
console_task = task_alloc();
/* 入口 console_main，独立栈、LEVEL… */
task_run(console_task, …);
```

**Console 与 B0～B2 一样后台并发** — 多任务架构 **自然延伸**。

#### Idle Task（闲置任务）

| 概念 | 说明 |
|------|------|
| **何时跑** | **无其他 runnable 任务** 时 |
| **优先级** | **最低 LEVEL** — 纯 **HLT 或空转** 占位 |
| **目的** | 调度器 **总有合法切换目标**；避免 **无任务可跑** 的边界 |

→ [Day 16 sleep/wake/LEVEL](../day-16-multitask2/)

---
