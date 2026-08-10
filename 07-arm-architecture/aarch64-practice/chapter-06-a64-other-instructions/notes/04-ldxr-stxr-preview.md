# 6.4 LDXR / STXR 独占访问（预览）

> 来源：§6.4 · 精读 · [章总览](section-0-本章完整概述.md)

## 本节讲什么

LDXR/STXR 独占加载/存储指令的预览——实现无锁原子操作的基础。

## 核心要点

```asm
; 原子自增
retry:
    ldxr w0, [x1]       ; 独占加载
    add  w0, w0, #1     ; 自增
    stxr w2, w0, [x1]   ; 独占存储，w2=0 成功，非0失败
    cbnz w2, retry      ; 失败则重试
```

- LDXR 设置独占监视器标记该地址
- STXR 尝试写回：监视器仍有效则成功(返回0)
- 如果中间有其他核写了该地址 → 监视器失效 → STXR 失败(返回非0)
- 通过循环重试实现原子 read-modify-write

> 详见 Ch20 原子操作。

## HFT 关联

LDXR/STXR 是无锁编程的基础：
- 实现无锁 ring buffer → 单生产者-单消费者模式避免争用
- 自旋锁用 LDXR/STXR 实现 → 但争用激烈时重试成本高
- HFT 中更推荐单线程模式（每个核独占数据）→ 完全避免原子操作
- ARMv8.1 LSE（LSE 原子指令）比 LDXR/STXR 在低争用时更快

## 自测题

1. LDXR/STXR 如何保证原子性？
<details><summary>答案</summary>
LDXR 在地址上设置独占监视器。STXR 检查监视器是否仍有效（没有其他核写该地址）。有效则写入成功(返回0)，无效则失败(返回非0)。通过循环重试直到成功，保证 read-modify-write 的原子性。
</details>

2. STXR 返回非 0 意味着什么？应该怎么处理？
<detail><summary>答案</summary>
返回非 0 表示独占写入失败（有其他核在 LDXR 和 STXR 之间写了该地址，监视器被清除）。需要重新 LDXR → 修改 → STXR 重试。通常用 CBNZ 判断并跳回重试。
</details>

3. LDXR/STXR 在高争用场景下有什么问题？
<detail><summary>答案</summary>
高争用下 STXR 频繁失败，导致大量重试循环 → 活锁风险（live-lock）、CPU 浪费、延迟不可预测。HFT 中应避免高争用原子操作，改用每核独立数据结构或单生产者-单消费者模式。
</details>

## 参考与延伸

- 原书 §6.4
- [3.6 特殊访存](../../chapter-03-a64-load-store/notes/section-0-本章完整概述.md)
- [Ch20 原子操作详解](../../chapter-20-atomic-operations/notes/section-0-本章完整概述.md)
