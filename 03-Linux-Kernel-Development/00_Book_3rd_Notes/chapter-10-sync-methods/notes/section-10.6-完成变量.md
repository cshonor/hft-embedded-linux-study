## ⑥ 完成变量 · Completion Variables

**两任务同步** — 一方 **等事件**，另一方 **发信号「完成了」**。

| 场景 | 示例 |
|------|------|
| 等待异步工作结束 | **`vfork()`** — 子进程就绪后 completion 唤醒父进程 |

```c
init_completion(&comp);
wait_for_completion(&comp);    /* 等待方 */
complete(&comp);               /* 完成方 */
```

与 mutex 区别：**一次性事件通知**，非保护共享数据临界区。

---
