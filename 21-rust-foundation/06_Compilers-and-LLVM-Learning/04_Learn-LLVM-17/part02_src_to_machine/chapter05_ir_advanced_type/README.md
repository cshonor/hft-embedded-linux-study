# Chapter 05 — 复合类型与高级结构在 IR 中的形态

| 项目 | 说明 |
|------|------|
| **学习策略** | **精读** |
| **对应书** | 《Learn LLVM 17》第 5 章 |

## 本目录用途

- 对照 **Rust 结构体、枚举、传参** 在 LLVM IR 中的 lowering（可配合从 `05-Async-Concurrency-Network/01-atomic/` 抄带 `struct` 的最小例进 `src/lib.rs` 再导出）。

## 目录约定

| 子目录 | 说明 |
|--------|------|
| `notes/` | 类型布局、`aggregate`、调用约定笔记 |
| `code/` | 可选：独立小 `struct` 实验说明 |

## IR 归档

`ir_samples/atomic_ir/` 或 `optimize_compare/`，命名示例：`ch05_struct_pass_rust_1.77.ll`。
