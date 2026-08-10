# §20.2 原子操作实现模式

> **来源：** [Ch20 完整概述](section-0-本章完整概述.md) · [章导读](../README.md)

## 本节讲什么

用 LDXR/STXR 实现原子加法、CAS（Compare-And-Swap）、交换。C++11 atomic 的各操作对应不同的 LDXR/STXR 循环模式。本节给出完整汇编代码和 C++ 映射。

## 核心要点

### 原子加法

```asm
; atomic_add(addr, val)
; x0 = addr, x1 = val
atomic_add:
1:  ldxr x2, [x0]       ; 独占读
    add  x2, x2, x1     ; 修改
    stxr w3, x2, [x0]   ; 独占写
    cbnz w3, 1b         ; 失败则重试
    ret
```

### CAS（Compare-And-Swap）

```asm
; x0 = addr, x1 = expected, x2 = desired
; 返回 x0: 旧值（=expected 表示成功）
cas:
1:  ldxr x3, [x0]       ; 独占读当前值
    cmp  x3, x1         ; 和期望值比较
    b.ne 2f             ; 不等 → 返回旧值
    stxr w4, x2, [x0]   ; 相等 → 写入新值
    cbnz w4, 1b         ; 写入失败 → 重试
2:  mov  x0, x3         ; 返回旧值
    ret
```

### 原子交换（Exchange）

```asm
; atomic_exchange(addr, val)
; x0 = addr, x1 = val
; 返回 x0: 旧值
atomic_exchange:
1:  ldxr x2, [x0]       ; 独占读旧值
    stxr w3, x1, [x0]   ; 独占写新值
    cbnz w3, 1b         ; 失败重试
    mov  x0, x2          ; 返回旧值
    ret
```

### C++11 atomic 对应

| C++ 操作 | ARM 实现 | 说明 |
|----------|----------|------|
| `atomic.load()` | `LDR`（配屏障）或 `LDAR` | 普通读 |
| `atomic.store()` | `STR`（配屏障）或 `STLR` | 普通写 |
| `atomic.fetch_add()` | `LDXR + ADD + STXR` 循环 | 原子加法 |
| `atomic.fetch_sub()` | `LDXR + SUB + STXR` 循环 | 原子减法 |
| `atomic.compare_exchange()` | `LDXR + CMP + STXR` 循环 | CAS |
| `atomic.exchange()` | `LDXR + STXR` 循环 | 交换 |
| `atomic.fetch_or()` | `LDXR + ORR + STXR` 循环 | 原子置位 |
| `atomic.fetch_and()` | `LDXR + AND + STXR` 循环 | 原子清位 |

### LDXR/STXR 循环通用模式

```
1:  ldxr Rd, [Rn]       ; 独占读
    <修改 Rd>            ; 操作（ADD/CMP/ORR/AND 等）
    stxr Rs, Rd, [Rn]   ; 独占写
    cbnz Rs, 1b         ; 失败重试
```

### 带屏障的原子操作

```asm
; 带 acquire/release 的原子加法（LDAXR/STLXR）
; atomic_add_return（返回新值，自带 acq_rel）
1:  ldaxr x2, [x0]      ; Load-Acquire Exclusive
    add  x2, x2, x1
    stlxr w3, x2, [x0]   ; Store-Release Exclusive
    cbnz w3, 1b
    mov  x0, x2           ; 返回新值
    ret
```

### CAS 失败时的 CLREX

```asm
; CAS 不匹配时清除监视器
1:  ldxr x3, [x0]
    cmp  x3, x1
    b.ne 2f
    stxr w4, x2, [x0]
    cbnz w4, 1b
    b 3f
2:  clrex                 ; ← 不匹配时清除监视器
3:  mov  x0, x3
    ret
```

## HFT 关联

CAS 是 HFT 无锁数据结构的核心原语——订单簿的并发更新可以用 CAS 实现：读取当前值 → 计算新值 → CAS 写入，失败则重试。

### HFT CAS 使用场景

```c
// 订单簿并发更新（MPSC）
bool update_order_book(OrderBook* book, const Order& ord) {
    PriceLevel old = book->levels[ord.level];
    PriceLevel new_val = apply_order(old, ord);
    // CAS：如果 level 没被其他线程修改，写入新值
    return atomic_cmpxchg(&book->levels[ord.level], old, new_val);
}

// 原子计数器（统计指标）
void record_order(int cpu) {
    // 每核独立计数器，无竞争
    per_cpu[cpu].order_count.fetch_add(1, std::memory_order_relaxed);
}

// 自旋锁（短临界区）
void spin_lock(atomic<int>* lock) {
    while (lock->exchange(1, std::memory_order_acquire) != 0)
        cpu_relax();  // 失败时让出 CPU
}
```

CAS 循环在高竞争时性能下降（反复失败重试），HFT 中应尽量用 SPSC（单生产者单消费者）模式避免竞争。`fetch_add` 用于原子计数器（如统计指标），在 ARM 上需要 LDXR/STXR 循环（~10-20ns），ARMv8.1 LSE 用单指令 LDADD 更快（~5ns）。

## 自测题

1. **写一个原子加法的汇编代码，解释每一步。**

<details>
<summary>答案</summary>

```asm
1:  ldxr x2, [x0]       ; 独占读当前值到 x2
    add  x2, x2, x1     ; x2 = x2 + x1（加法）
    stxr w3, x2, [x0]   ; 独占写回 x2 到 [x0]，w3=0成功/1失败
    cbnz w3, 1b         ; w3≠0 → 失败，跳回 1 重试
    ret
```

LDXR 标记独占，ADD 修改，STXR 检查独占是否被干扰。如果其他核在此期间写了该地址，STXR 返回 1，cbnz 跳回重试。
</details>

2. **CAS 操作中，如果 `cmp x3, x1` 发现不等（值不匹配），应该怎么处理？**

<details>
<summary>答案</summary>

值不匹配时**不执行 STXR**，直接返回旧值 x3。但应该执行 `CLREX` 清除独占监视器（LDXR 标记了独占但不写入）。不执行 CLREX 的话监视器残留，可能影响后续 LDXR。在某些 ARM 实现中，CLREX 是可选的（监视器会自动过期），但最佳实践是显式 CLREX。
</details>

3. **`atomic.fetch_add()` 在 ARM 上编译成什么指令序列？**

<details>
<summary>答案</summary>

编译为 `LDXR + ADD + STXR + CBNZ` 循环：
```asm
1:  ldxr x2, [x0]       ; 独占读
    add  x2, x2, x1     ; 加法
    stxr w3, x2, [x0]   ; 独占写
    cbnz w3, 1b         ; 失败重试
```
如果 CPU 支持 ARMv8.1 LSE，编译器可能用 `LDADD` 单指令替代循环（更快）。Linux 通过 `__LSE_ATOMIC` 宏在编译期选择。
</details>

4. **`atomic_add`（不带返回值）和 `atomic_add_return`（带返回值）在 ARM 上的指令有什么区别？**

<details>
<summary>答案</summary>

- `atomic_add`（不带返回值）：用 `LDXR + STXR`（普通独占），不带 acquire/release
- `atomic_add_return`（带返回值）：用 `LDAXR + STLXR`（带 acquire/release 的独占），自带屏障

原因：`atomic_add_return` 需要返回新值，调用者可能依赖新值的可见性顺序，所以需要 acquire/release。`atomic_add` 不返回值，等价 `memory_order_relaxed`。
</details>

## 参考与延伸

- [§20.1 独占监视器](01-exclusive-monitor.md) — LDXR/STXR 原理
- [§20.3 ARMv8.1 LSE](03-lse.md) — 单指令原子操作
- [§20.5 Linux 原子操作 API](05-linux-atomic-api.md) — 内核封装
