## ① 任务管理自动化 · 封装与重构

Day 15：**TSS 初始化、GDT 项、far-JMP** 堆在 **`HariMain`** — 加第三个任务就要 **复制大段代码**。

#### `TASKCTL` + `mtask.c`

| 组件 | 职责 |
|------|------|
| **`TASKCTL`** | 任务控制块集合 — 当前任务、运行队列、层级表… |
| **`task_alloc()`** | 分配 TSS、栈、GDT 槽，**初始化任务结构** |
| **`task_run()`** | 把任务 **挂入运行队列**，参与调度 |

**主程序以后：**

```c
task = task_alloc();
/* 设 entry、栈、level… */
task_run(task, priority, level);
```

**底层细节进内核模块** — 与 Linux **`fork/schedule`** 分层同构（尺度小很多）。

→ [Day 15 TSS/far-JMP](../day-15-multitask1/)

---
