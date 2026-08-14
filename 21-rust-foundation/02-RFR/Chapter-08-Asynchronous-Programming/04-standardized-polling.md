# 1.4 Standardized Polling（标准化轮询）

> 所属：**What's the Deal with Asynchrony?** · [← 章索引](./README.md)

`Future::poll(&mut self, cx: &mut Context<'_>) -> Poll<Output>` 是生态统一接口：

- **`Context`** 携带 **`Waker`**
- 执行器反复 `poll` 直到 `Ready` 或任务挂起

不同运行时（Tokio、async-std 等）共享此契约，差异在调度与 I/O 驱动实现。

→ [08 Poll 契约](./08-poll-contract.md) · [07 唤醒](./07-waking-up.md)
