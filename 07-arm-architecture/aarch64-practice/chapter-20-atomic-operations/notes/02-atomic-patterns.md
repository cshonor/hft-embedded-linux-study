# §20.2 原子操作实现模式

> **来源：** [Ch20 完整概述](section-0-本章完整概述.md) · [章导读](../README.md)

## 本节讲什么

用 LDXR/STXR 实现原子加法、CAS（Compare-And-Swap）、交换。C++11 atomic 的各操作对应不同的 LDXR/STXR 循环模式。

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

### C++11 atomic 对应

| C++ 操作 | ARM 实现 |
|----------|----------|
| `atomic.load()` | `LDR`（配屏障）或 `LDAR` |
| `atomic.store()` | `STR`（配屏障）或 `STLR` |
| `atomic.fetch_add()` | `LDXR + ADD + STXR` 循环 |
| `atomic.compare_exchange()` | `LDXR + CMP + STXR` 循环 |
| `atomic.exchange()` | `LDXR + STXR` 循环 |

### LDXR/STXR 循环模式总结

```
1:  ldxr Rd, [Rn]       ; 独占读
    <修改 Rd>            ; 操作（ADD/CMP/等）
    stxr Rs, Rd, [Rn]   ; 独占写
    cbnz Rs, 1b         ; 失败重试
```

## HFT 关联

CAS 是 HFT 无锁数据结构的核心原语——订单簿的并发更新可以用 CAS 实现：读取当前值 → 计算新值 → CAS 写入，失败则重试。CAS 循环在高竞争时性能下降（反复失败重试），HFT 中应尽量用 SPSC（单生产者单消费者）模式避免竞争。`fetch_add` 用于原子计数器（如统计指标），在 ARM 上需要 LDXR/STXR 循环（~10-20ns），ARMv8.1 LSE 用单指令 LDADD 更快（~5ns）。

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

## 参考与延伸

- [§20.1 独占监视器](01-exclusive-monitor.md) — LDXR/STXR 原理
- [§20.3 ARMv8.1 LSE](03-lse.md) — 单指令原子操作
- [§20.5 Linux 原子操作 API](05-linux-atomic-api.md) — 内核封装
