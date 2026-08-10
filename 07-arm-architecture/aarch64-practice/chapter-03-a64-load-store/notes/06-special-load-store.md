# 3.6 特殊访存指令

> 来源：§3.6 · 精读 · [章总览](section-0-本章完整概述.md)

## 本节讲什么

LDXR/STXR 独占访存、LDAR/STLR acquire/release 访存、LDTR/STTR 非特权访存。

## 核心要点

| 指令 | 用途 |
|------|------|
| LDXR/STXR | 独占加载/存储；监视器 + 失败返回非 0 → CAS/自旋锁原语 |
| LDAR/STLR | Load-Acquire/Store-Release（自带内存序语义） |
| LDTR/STTR | 非特权访问；EL1 可用 EL0 权限碰用户地址 |

- LDXR/STXR 实现无锁原子操作（Ch20 详解）
- LDAR/STLR 是 ARMv8 的 acquire/release 语义指令（Ch18 详解）
- LDTR 让内核以用户权限检查地址可访问性（copy_from_user 场景）

## HFT 关联

这些指令对无锁编程至关重要：
- LDXR/STXR 实现无锁队列/ring buffer → 避免 mutex 的上下文切换开销
- 但 LDXR/STXR 在争用激烈时重试成本高 → HFT 中用单生产者-单消费者模式避免争用
- LDAR/STLR 比显式 DMB 屏障更精确，只排序相关访问 → 减少不必要的屏障开销
- LDTR 用于内核安全地复制用户数据（如交易指令从用户态传入），防止绕过权限检查

## 自测题

1. LDXR/STXR 如何实现原子操作？
<details><summary>答案</summary>
LDXR 设置独占监视器标记该地址。STXR 尝试写回：如果监视器仍持有独占标记则写入成功（返回 0）；如果中间有其他核写了该地址则失败（返回非 0）。通过循环重试实现原子 read-modify-write。
</details>

2. LDAR 和 LDR 的区别是什么？
<details><summary>答案</summary>
LDAR = Load-Acquire，保证 LDAR 之后的读写不会重排到 LDAR 之前。LDR 是普通加载，没有内存序保证。LDAR 用于需要 acquire 语义的场景（如读取标志后访问对应数据）。
</details>

3. 为什么内核用 LDTR 而不是 LDR 来访问用户空间地址？
<details><summary>答案</summary>
LDTR 以 EL0（用户）权限执行访问，即使用户空间的页表设置了 PXN（特权不可执行）等限制，内核也能正确检测到权限违规。用普通 LDR 则以内核权限访问，可能绕过用户空间的访问限制。
</details>

## 参考与延伸

- 原书 §3.6
- [Ch18 内存屏障](../../chapter-18-memory-barriers/notes/section-0-本章完整概述.md)
- [Ch20 原子操作](../../chapter-20-atomic-operations/notes/section-0-本章完整概述.md)
