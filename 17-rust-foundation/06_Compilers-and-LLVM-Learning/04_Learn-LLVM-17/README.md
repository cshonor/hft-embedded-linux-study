# 04 · Learn LLVM 17 · LLVM IR 透视

> 所属：[Compilers & LLVM Learning](../README.md)（仓库编号 **06**）  
> **C++ 前置（必修）**：本仓 `04-cpp` **最小子集 P0**（非姊妹仓 01～06，见下） → 见 [06/README 前置说明](../README.md#开-learn-llvm-前的-c-前置必修)  
> 与 RFR **第 2、第 10 章** 对照读 IR；**本目录用 Rust 导出 IR，不必写 C++ Pass**。  
> 前置实战：[05-Async-Concurrency-Network](../../05-Async-Concurrency-Network/README.md)

**笔记 + 可运行 crate `llvm_insight_lab` + `ir_samples/`** 分目录完成。

> 🗑️ **2026-09-10 清理**：`part04_custom_backend`（第 11 / 12 / 13 章）与 `part03/chapter08_tablegen`（第 8 章）
> 均属 [《学习取舍》](./Learn-LLVM-17-学习取舍.md) 判定的「**整段跳过**」，原为不足 500 字节的空壳，已删除。
> 剩余 `part01` / `part02` / `part03`（ch09、ch10）中仍有空壳待补。

---

## 统一 IR 实验 crate

包名：**`llvm_insight_lab`**。

在**仓库根**执行：

```bash
cargo build --manifest-path 06_Compilers-and-LLVM-Learning/04_Learn-LLVM-17/Cargo.toml
cargo rustc --manifest-path 06_Compilers-and-LLVM-Learning/04_Learn-LLVM-17/Cargo.toml -- --emit=llvm-ir
```

生成物通常在 `target/debug/deps/llvm_insight_lab-*.ll`。片段复制到 `ir_samples/`（见 [ir_samples/README.md](./ir_samples/README.md)）。

### 建议对照实验

1. 改 `src/lib.rs` 中 `Ordering`，diff O0/O3 → `ir_samples/optimize_compare/`。
2. 从 [04/01-atomic](../../05-Async-Concurrency-Network/01-atomic/) 摘短逻辑 → `ir_samples/atomic_ir/`。
3. 从 [04/02-async_tokio](../../05-Async-Concurrency-Network/02-async_tokio/) 导出 → `ir_samples/async_tokio_ir/`。

---

## 速记

**精读** ch02 · ch04 · ch05 · ch07 · ch10 · **浏览** ch01/03/06/09 · **跳过** ch08 · part04  
**取舍详表** → [Learn-LLVM-17-学习取舍.md](./Learn-LLVM-17-学习取舍.md)
