## ⑥ 完成变量 · Completions

表达 **「某事件做完了」** — 一方等待完成，另一方发出完成信号。比「乱用等待队列 + 条件变量手搓」更贴语义。

| 角色 | 动作 |
|------|------|
| **等待方** | `wait_for_completion()` — 睡到完成 |
| **完成方** | `complete()` / `complete_all()` — 唤醒等待者 |

#### 典型场景

| 场景 | 例子 |
|------|------|
| 父等子做到某步 | 历史上与 `vfork` 等故事相关 |
| 驱动等硬件初始化线程 | 探针里等工作线程 `complete` |
| 模块卸载等引用归零 | 「最后一用户走了」 |

```
线程 A:  start work ──► wait_for_completion(&done)
线程 B:  ... finish ... ──► complete(&done)
              │
              └──► A 被唤醒继续
```

#### 与信号量/等待队列

| | completion | 裸 wait queue |
|--|------------|---------------|
| 语义 | **一次性/完成事件** 清晰 | 通用，易写错 |
| 多次 complete | `complete_all` 等变体 | 自己维护标志 |

注意：等待方通常在 **进程上下文**；完成方可以在原子上下文 `complete`（具体 API 约束以头文件为准）— 但等待方仍不能在 ISR 里 `wait_for_completion`。

**HFT：** 用户态 `promise/future`、一次性 latch 同类；热路径少用「等完成」睡眠，用无锁标志 + 忙等/轮询仅限微秒级且可证明正确。

→ [4.4 休眠唤醒](../chapter-04-process-scheduling/notes/section-4.4-休眠与唤醒.md) · [10.5 mutex](./section-10.5-互斥体.md)

---
