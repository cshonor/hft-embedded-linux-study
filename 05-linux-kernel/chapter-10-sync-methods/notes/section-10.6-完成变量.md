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

→ [4.4 休眠唤醒](../../chapter-04-process-scheduling/notes/section-4.4-休眠与唤醒.md) · [10.5 mutex](./section-10.5-互斥体.md)

### 常见陷阱

1. 混淆 completion 和 semaphore——completion 是一次性通知，semaphore 可重复
2. 在 completion 的 wait_for_completion() 中以为会自旋——它会睡眠（进程上下文）
3. 多次 complete() 一个 completion——complete() 通常只调一次，complete_all() 标记永久完成

---

<details>
<summary>自测题（点击展开）</summary>

**Q1.** completion 的典型使用场景？

<details><summary>答案</summary>

驱动初始化等硬件就绪：`init_completion(&done)` → 启动硬件 → `wait_for_completion_timeout(&done, timeout)` → 中断处理函数中 `complete(&done)`。线程池任务完成通知：主线程 `wait_for_completion()` 等所有 worker `complete()`。模块卸载等引用归零。vs semaphore：completion 语义更清晰（一次性事件），semaphore 适合计数资源。

</details>

**Q2.** complete() 和 complete_all() 的区别？

<details><summary>答案</summary>

complete()：唤醒一个等待者，completion 的 done+1。如果有多个等待者，需要多次 complete()。complete_all()：唤醒所有等待者，并将 completion 标记为永久完成（后续 wait_for_completion() 立即返回）。complete_all() 后不能重用该 completion（除非 reinit）。典型：驱动 probe 成功后 complete_all()，所有等待者放行。

</details>

**Q3.** HFT 中 completion 的用户态对应物？

<details><summary>答案</summary>

① `std::promise<T>` + `std::future<T>`：一次性设置值 + 等待。② `std::condition_variable`：更灵活，可重复使用。③ `std::latch`（C++20）：一次性，多线程等同一事件。④ `std::barrier`（C++20）：多线程同步点。HFT 热路径不用这些（有 syscall 开销），用无锁标志位（`std::atomic<bool>` + spin）。

</details>

</details>

---
