# §20.5 Linux 原子操作 API

> **来源：** [Ch20 完整概述](section-0-本章完整概述.md) · [章导读](../README.md)

## 本节讲什么

Linux 内核的原子操作 API：atomic_read/set、atomic_add、atomic_cmpxchg 等。底层根据配置选择 LDXR/STXR 循环或 LSE 单指令。原子操作不自带内存屏障，需要额外加。

## 核心要点

### Linux 原子操作 API

| API | 实现 | 说明 |
|-----|------|------|
| `atomic_read(v)` | `READ_ONCE` | 原子读 |
| `atomic_set(v, i)` | `WRITE_ONCE` | 原子写 |
| `atomic_add(i, v)` | `stxr` 循环 / `LDADD` | 原子加 |
| `atomic_add_return(i, v)` | `LDAXR/STLXR` 循环 | 原子加并返回新值 |
| `atomic_cmpxchg(v, old, new)` | `LDXR/CMP/STXR` | CAS |
| `atomic_inc(v)` | `atomic_add(1, v)` | 原子自增 |
| `smp_mb__after_atomic()` | `dmb ish` | 原子操作后加屏障 |

### 带返回值 vs 不带返回值

| API | 屏障 | 用途 |
|-----|------|------|
| `atomic_add(i, v)` | 不带（LDXR/STXR） | 只需要原子加，不关心返回值 |
| `atomic_add_return(i, v)` | 带（LDAXR/STLXR） | 需要返回新值，自带 acquire/release |
| `atomic_cmpxchg(v, old, new)` | 不带（LDXR/STXR） | CAS，需要手动加屏障 |

> **关键**：`atomic_add`（不带 return）**不自带屏障**——不保证和其他访存的顺序。如果需要顺序保证，用 `smp_mb__after_atomic()`。

### atomic_read 的陷阱

```c
// atomic_read 只保证编译器不优化，不保证多核可见性！
int val = atomic_read(&counter);  // ← 可能读到旧值
// 需要：
smp_rmb();  // 或用 smp_load_acquire()
int val = smp_load_acquire(&counter);
```

## HFT 关联

Linux 原子 API 在用户态 HFT 中不直接可用，但理解其设计有助于正确使用 C++ `std::atomic`。关键点：1) `atomic_add`（不带 return）不自带屏障，等价于 C++ `memory_order_relaxed`；2) `atomic_add_return` 自带 acquire/release，等价于 `memory_order_acq_rel`；3) `smp_mb__after_atomic()` 用于在 relaxed 原子操作后补屏障。HFT 中应优先用 `std::atomic` 的内存序参数明确指定语义，而非依赖 Linux API 的隐含行为。

## 自测题

1. **`atomic_add(i, v)` 和 `atomic_add_return(i, v)` 的区别？哪个自带屏障？**

<details>
<summary>答案</summary>

- `atomic_add(i, v)`：原子加，**不返回新值**，用 LDXR/STXR（不带屏障）。等价 C++ `memory_order_relaxed`。
- `atomic_add_return(i, v)`：原子加并**返回新值**，用 LDAXR/STLXR（自带 acquire/release）。等价 C++ `memory_order_acq_rel`。

如果需要顺序保证但不关心返回值，用 `atomic_add` + `smp_mb__after_atomic()`。
</details>

2. **`atomic_read` 能保证读到最新值吗？为什么？**

<details>
<summary>答案</summary>

**不能**。`atomic_read` 展开为 `READ_ONCE`（`volatile` 读），只阻止编译器优化（不缓存到寄存器），不保证多核可见性——其他核的写可能还没对这个核可见（ARM 弱序模型）。要保证读到最新值，需要 `smp_rmb()` 或用 `smp_load_acquire()`（编译为 LDAR，带 acquire 语义）。
</details>

3. **`smp_mb__after_atomic()` 的作用是什么？什么时候需要？**

<details>
<summary>答案</summary>

`smp_mb__after_atomic()` = `dmb ish`，在原子操作后补全屏障。需要用的场景：`atomic_add`（不带 return）不自带屏障，如果原子操作后需要保证之前的访存已完成对其他核可见，用 `smp_mb__after_atomic()` 补屏障。例如：
```c
data = 42;
atomic_inc(&counter);
smp_mb__after_atomic();  // 保证 data=42 在 atomic_inc 后对其他核可见
```
如果用 `atomic_add_return`（自带屏障）则不需要。
</details>

## 参考与延伸

- [§20.2 原子操作实现模式](02-atomic-patterns.md) — LDXR/STXR 循环
- [§20.3 ARMv8.1 LSE](03-lse.md) — LDADD 等单指令
- [Ch18 §18.5 Linux 内核屏障 API](../../chapter-18-memory-barriers/notes/section-0-本章完整概述.md) — smp_mb 等详解
