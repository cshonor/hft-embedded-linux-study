# 4.5 The Fetch Methods（Fetch 方法）

> 所属：**Lower-Level Concurrency** · [← 章索引](./README.md)

**`fetch_add` / `fetch_sub` / `fetch_or` / …** — 原子 RMW，返回**旧值**。

- 指定 **Ordering**（常见 `AcqRel` + `Acquire` failure order）
- 计数器、位图、简单统计

与 **CAS 循环** 相比：单指令 RMW 在硬件上可能更高效，但语义更窄。

`06_Compilers-and-LLVM-Learning/04_Learn-LLVM-17` → diff `Relaxed` vs `SeqCst` 的 `.ll` 片段。
