# 2.3 Test Augmentation（测试增强）

> 所属：**Additional Testing Tools** · [← 章索引](./README.md)

## Miri

**MIR 解释器**：`cargo miri test` 较慢，但可检测：

- 未初始化读、悬垂解引用
- 违反 stacked borrows / 别名规则等 **UB**

把 `unsafe` 问题从「生产偶现」拉到「测试可复现」。

→ [ER Item 16 demo](../../01-ER/Chapter-03-Concepts/Item-16-avoid-unsafe/demo/)

## Loom

并发 bug 依赖**线程交错**。**Loom** 接管 `Mutex`、原子等语义，**系统化探索**调度顺序，暴露极端交错下的竞争与死锁。

与 [RFR 第 10 章](../Chapter-10-Concurrency-and-Parallelism/10-并发与并行-Concurrency-and-Parallelism-深度解析.md) 衔接。

## 06_Compilers-and-LLVM-Learning/04_Learn-LLVM-17

排错时可对照 IR / 优化是否改变测试假设 → [06_Compilers-and-LLVM-Learning/04_Learn-LLVM-17](../../06_Compilers-and-LLVM-Learning/04_Learn-LLVM-17/README.md)
