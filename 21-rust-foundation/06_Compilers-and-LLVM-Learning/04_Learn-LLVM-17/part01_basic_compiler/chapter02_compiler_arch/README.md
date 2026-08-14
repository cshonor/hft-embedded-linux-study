# Chapter 02 — 编译器整体架构

| 项目 | 说明 |
|------|------|
| **学习策略** | **精读** |
| **对应书** | 《Learn LLVM 17》第 2 章 |

## 本目录用途

- **三段式**：源码 → AST → IR → 机器码；**rustc** 中 HIR/MIR 与 LLVM IR 的对应关系（不必背全，能画一张草图即可）。  
- 与仓库：`rustc` 如何把 `05-Async-Concurrency-Network/01-atomic/`、`05-Async-Concurrency-Network/02-async_tokio/` 里的 Rust 降到 LLVM。

## 目录约定

| 子目录 | 说明 |
|--------|------|
| `notes/` | 精读笔记、流程图链接、书中术语表 |
| `code/` | 可选：极简「伪前端」笔记代码或外链，非必须 |

## IR 归档

本章一般**不产出** `.ll`；若做迷你实验，将样本放入 `ir_samples/optimize_compare/` 并注明与 ch02 的对应关系。
