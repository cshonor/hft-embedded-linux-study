# 1.1 Enumeration（枚举式错误）

> 所属：**Representing Errors** · [← 章索引](./README.md)

当调用者**需要按错误种类采取不同行动**时，用 `enum` 表达失败原因。

## 何时用

- 复制管道：输入断开 vs 输出断开 → 策略不同。
- 库 API：调用方要 `match` 分支重试、降级、换路径。

## 生态友好 checklist

1. **`std::error::Error`** + `source()` — 因果链、与日志/backtrace 协作。
2. **`Display`** — 单行、小写、句末无多余标点，便于嵌入日志。
3. **`Debug`** — 路径、端口等诊断细节。
4. **`Send + Sync`** — 异步 / 多线程传播。
5. **`'static`** — 减少生命周期纠缠；便于与 opaque 类型组合。

## 工具

- **`thiserror`** — 派生 + 变体 + `source` 映射 → [ER Item 04](../../01-ER/Chapter-01-Types/Item-04-idiomatic-error-types/README.md)
- **错误链精读** — `Error::source()`、`anyhow::chain()`、与 panic/join 对照 → [01 错误链](./01-error-source-chain.md)
- **线程 panic 容器层** — `Box<dyn Any>` / downcast（挂到通用链下，不重复正文）→ [1.1.2.3](../../05-Async-Concurrency-Network/01-atomic/Chapter-01-Rust-Concurrency-Basics/1.1-threads-in-rust/1.1.2.3-panic-box-dyn-any.md)
