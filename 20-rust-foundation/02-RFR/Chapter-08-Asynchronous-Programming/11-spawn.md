# 4. Tying It All Together with spawn

> [← 章索引](./README.md)

若在单循环里**顺序 `.await`** 每个连接，**并发度为 1**。

## `spawn`

把每个连接的 `Future` 变成**独立顶层任务**，由运行时线程池调度 → accept 与业务处理**重叠**。

## `Send` 约束

跨线程 spawn 通常要求 **`Future: Send`**（捕获的数据须 `Send`）。

Pin + Send 组合是 Tokio 报错高频区 → [06 Pin/Unpin](./06-pin-unpin.md) · [第 10 章 Send/Sync](../Chapter-10-Concurrency-and-Parallelism/01-correctness.md)
