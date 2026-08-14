# 1.1 Synchronous Interfaces（同步接口）

> 所属：**What's the Deal with Asynchrony?** · [← 章索引](./README.md)

CPU 很快，程序大量时间在 **等待**（I/O、网络、磁盘、事件）。

**同步模型**：调用线程**阻塞**直到 I/O 完成。

| 优点 | 代价 |
|------|------|
| 心智简单、栈上局部变量自然 | 每连接一线程 → 阻塞成本放大 |

适合低并发、原型、CPU  bound 短任务。
