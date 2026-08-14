# 1.3 Asynchronous Interfaces（异步接口）

> 所属：**What's the Deal with Asynchrony?** · [← 章索引](./README.md)

**异步模型**：等待时**让出**线程给其他任务；I/O 就绪后再继续。

| 优点 | 代价 |
|------|------|
| 少量线程高并发 | 状态机、`Pin`、取消、背压 |

Rust 核心仍是 **`Future::poll`**：未就绪 → **`Poll::Pending`**，就绪 → **`Ready`**。

→ [04 标准化轮询](./04-standardized-polling.md)
