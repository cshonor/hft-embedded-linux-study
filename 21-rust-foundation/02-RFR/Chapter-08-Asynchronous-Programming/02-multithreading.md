# 1.2 Multithreading（多线程）

> 所属：**What's the Deal with Asynchrony?** · [← 章索引](./README.md)

**多线程模型**：某线程阻塞时，OS 调度其他线程运行。

| 优点 | 代价 |
|------|------|
| 仍用阻塞式 API | 线程栈、切换、同步（锁、原子、数据竞争） |

→ [第 10 章](../Chapter-10-Concurrency-and-Parallelism/README.md) · Book [16.1 线程](../../00-Book/16-fearless-concurrency/16.1-使用线程同时运行代码.md)
