# 3. Asynchrony and Parallelism（异步与并行）

> [← 章索引](./README.md)

| 概念 | 含义 |
|------|------|
| **并发 (concurrency)** | 任务**交错**；单线程 `async` 即可 |
| **并行 (parallelism)** | 任务**同时**执行；需多核 + 多线程 |

## 吃满多核

异步 Web 等：执行器 **`spawn`** 多个 **`Send`** 任务 → futures 分布到线程池。

→ [第 8 章 spawn](../Chapter-08-Asynchronous-Programming/11-spawn.md)

## 异步中的锁

`std::sync::Mutex` **跨 `.await`** → 长时间阻塞 worker。

更常见：**`tokio::sync::Mutex`** 等异步锁 — 注意持锁粒度与开销。
