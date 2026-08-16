# 3.2 Fulfilling the Poll Contract（Poll 契约）

> 所属：**Going to Sleep** · [← 章索引](./README.md)

## 契约

返回 **`Pending`** 时，必须确保将来某处会 **`wake`**（除非任务被永久取消且协议允许）—— 否则任务**饿死**。

## 不要在 executor 上阻塞

在 worker 上 **`thread::sleep`**、长 CPU 死算、**同步阻塞 I/O** → 占用 worker，同池任务饥饿。

**经验法则**：`poll` 路径避免长时间独占线程（阈值依运行时与 SLA 而定）。

异步里持 **`std::sync::Mutex` 跨 `.await`** 同理 → [第 10 章 · 异步与锁](../Chapter-10-Concurrency-and-Parallelism/06-asynchrony-parallelism.md)
