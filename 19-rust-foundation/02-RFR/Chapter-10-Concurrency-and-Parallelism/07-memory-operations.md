# 4.1 Memory Operations（内存操作）

> 所属：**Lower-Level Concurrency** · [← 章索引](./README.md)

## 物理现实

- 编译器可**重排**
- CPU 可**乱序**
- 各核缓存**非瞬时一致**

单条指令的原子性**不足以**表达跨线程的**可见性**契约 → 需要 **memory ordering**。

→ [09 内存序](./09-memory-ordering.md) · [06_Compilers-and-LLVM-Learning/04_Learn-LLVM-17](../../06_Compilers-and-LLVM-Learning/04_Learn-LLVM-17/part02_src_to_machine/chapter07_ir_optimize/README.md)
