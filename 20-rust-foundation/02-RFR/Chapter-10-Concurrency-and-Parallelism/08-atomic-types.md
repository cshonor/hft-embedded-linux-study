# 4.2 Atomic Types（原子类型）

> 所属：**Lower-Level Concurrency** · [← 章索引](./README.md)

**`std::sync::atomic`** — `AtomicUsize`、`AtomicBool`、`AtomicPtr` 等。

- 读/写/RMW 在硬件层原子（对对齐地址）。
- 须配对正确的 **Ordering** → [09](./09-memory-ordering.md)

实验 → [06_Compilers-and-LLVM-Learning/04_Learn-LLVM-17/src/lib.rs](../../06_Compilers-and-LLVM-Learning/04_Learn-LLVM-17/src/lib.rs) 改 `Ordering` 导出 IR diff。

Book · `05-Async-Concurrency-Network/01-atomic/` 仓库（狗熊书）深化。
