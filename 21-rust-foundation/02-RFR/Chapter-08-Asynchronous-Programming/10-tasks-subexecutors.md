# 3.4 Tasks and Subexecutors（任务与子执行器）

> 所属：**Going to Sleep** · [← 章索引](./README.md)

运行时把 `Future` 包装为 **Task**，由 **executor / worker 池** 调度。

## 层级

- **顶层任务** — 可被 `spawn` 独立调度
- **子 Future** — 嵌在父 `poll` 内，由父驱动

## 子执行器 (subexecutor)

部分运行时允许在任务内嵌局部调度逻辑；细节因 Tokio / async-std 等而异 — 读文档时对照「task / join handle / LocalSet」等概念。

→ 与 [11 spawn](./11-spawn.md) 衔接
