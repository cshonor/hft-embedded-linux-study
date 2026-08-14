# Rustonomicon 学习仓库

配套 **[The Rustonomicon](https://doc.rust-lang.org/nomicon/)** — 围绕 *The Dark Arts of Unsafe Rust* 的 10 个主题板块。

## 基准 Edition

**2018**（与 Nomicon 主体一致；后续可按需升级单章 crate）

## 全书大纲

| 目录 | 官方主题 |
|------|----------|
| [01_Safe_Unsafe/](./01_Safe_Unsafe/) | Meet Safe and Unsafe |
| [02_Data_Layout/](./02_Data_Layout/) | Data Representation in Rust |
| [03_Lifetime_Variance/](./03_Lifetime_Variance/) | Ownership and Lifetimes |
| [04_Type_Cast/](./04_Type_Cast/) | Type Conversions |
| [05_Uninit_Mem/](./05_Uninit_Mem/) | Working With Uninitialized Memory |
| [06_OBRM_RAII/](./06_OBRM_RAII/) | The Perils Of OBRM |
| [07_Concurrency_Atomic/](./07_Concurrency_Atomic/) | Concurrency and Parallelism |
| [08_Impl_Vec_Arc/](./08_Impl_Vec_Arc/) | Implementing Vec / Arc and Mutex |
| [09_FFI/](./09_FFI/) | Foreign Function Interface |
| [10_NoStd/](./10_NoStd/) | Beneath std |

| 资料 | 路径 |
|------|------|
| 全书进度 | [notes.md](./notes.md) |
| 各章笔记 + 源码 | 各章 `00-overview.md` + `Cargo.toml` |

## 编译示例

```bash
cd 04-Rust-Nomicon/01_Safe_Unsafe
cargo run
```

与本仓库其它板块对照：[`03-DeepRustStdLib/`](../03-DeepRustStdLib/README.md)（标准库进阶）· [`05-Async-Concurrency-Network/`](../05-Async-Concurrency-Network/README.md)（`01-atomic/` Ch07 并发 · `02-async_tokio/` · `rust_network_programming/stage08` TLS）· [`02-RFR/`](../02-RFR/RFR-本书目录.md)（类型与 unsafe 进阶）。
