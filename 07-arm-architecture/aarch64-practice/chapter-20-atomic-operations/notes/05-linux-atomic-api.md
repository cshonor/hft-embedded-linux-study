# §20.5 Linux 原子操作 API

> **来源：** [Ch20 完整概述](section-0-本章完整概述.md) · [章导读](../README.md)

## 本节讲什么

Linux 内核的原子操作 API：atomic_read/set、atomic_add、atomic_cmpxchg 等。底层根据配置选择 LDXR/STXR 循环或 LSE 单指令。原子操作不自带内存屏障，需要额外加。本节给出完整 API 对照和屏障补充方法。

## 核心要点

### Linux 原子操作 API 完整表

| API | 实现 | 屏障 | 等价 C++ | 说明 |
|-----|------|------|---------|------|
| `atomic_read(v)` | `READ_ONCE` | 无 | relaxed | 原子读 |
| `atomic_set(v, i)` | `WRITE_ONCE` | 无 | relaxed | 原子写 |
| `atomic_add(i, v)` | LDXR/STXR 或 LDADD | 无 | relaxed | 原子加 |
| `atomic_add_return(i, v)` | LDAXR/STLXR | acq_rel | acq_rel | 原子加返回新值 |
| `atomic_sub(i, v)` | LDXR/STXR | 无 | relaxed | 原子减 |
| `atomic_sub_return(i, v)` | LDAXR/STLXR | acq_rel | acq_rel | 原子减返回新值 |
| `atomic_cmpxchg(v, old, new)` | LDXR/CMP/STXR | 无 | relaxed CAS | CAS |
| `atomic_xchg(v, new)` | LDXR/STXR | 无 | relaxed | 交换 |
| `atomic_inc(v)` | atomic_add(1,v) | 无 | relaxed | 自增 |
| `atomic_inc_return(v)` | atomic_add_return(1,v) | acq_rel | acq_rel | 自增返回 |
| `atomic_dec(v)` | atomic_sub(1,v) | 无 | relaxed | 自减 |
| `atomic_or(i, v)` | LDXR/ORR/STXR | 无 | relaxed | 原子或 |
| `atomic_and(i, v)` | LDXR/AND/STXR | 无 | relaxed | 原子与 |

### 带返回值 vs 不带返回值

| API | 屏障 | 指令 | 用途 |
|-----|------|------|------|
| `atomic_add(i, v)` | 不带 | LDXR/STXR | 只需原子加，不关心返回值 |
| `atomic_add_return(i, v)` | 带 | LDAXR/STLXR | 需要返回新值，自带 acq_rel |
| `atomic_cmpxchg(v, old, new)` | 不带 | LDXR/CMP/STXR | CAS，需要手动加屏障 |
| `atomic_cmpxchg_acquire(v, old, new)` | acquire | LDAXR/CMP/STLXR | CAS 自带 acquire |

> **关键**：`atomic_add`（不带 return）**不自带屏障**——不保证和其他访存的顺序。如果需要顺序保证，用 `smp_mb__after_atomic()`。

### 屏障补充方法

| 场景 | API | 说明 |
|------|-----|------|
| 原子操作后需要全屏障 | `smp_mb__after_atomic()` | 补 `dmb ish` |
| 原子操作前需要全屏障 | `smp_mb__before_atomic()` | 补 `dmb ish` |
| 原子读需要 acquire | `smp_load_acquire(p)` | 编译为 LDAR |
| 原子写需要 release | `smp_store_release(p, v)` | 编译为 STLR |
| 原子读只防编译器 | `READ_ONCE(p)` | `volatile` 读 |
| 原子写只防编译器 | `WRITE_ONCE(p, v)` | `volatile` 写 |

### atomic_read 的陷阱

```c
// atomic_read 只保证编译器不优化，不保证多核可见性！
int val = atomic_read(&counter);  // ← 可能读到旧值
// 需要：
smp_rmb();  // 或用 smp_load_acquire()
int val = smp_load_acquire(&counter);  // → LDAR

// 正确的"写后读"模式
data = 42;
atomic_inc(&ready);
smp_mb__after_atomic();  // 保证 data=42 在 atomic_inc 后可见
```

### smp_mb__after_atomic 使用场景

```c
// 场景1：原子操作后需要其他访存可见
data = 42;
atomic_inc(&counter);
smp_mb__after_atomic();  // 保证 data 在 counter++ 后对其他核可见
// 其他核：读 counter 看到新值后，data 一定是 42

// 场景2：替代 seq_cst
// C++: atomic.fetch_add(1, std::memory_order_seq_cst);
// Linux: atomic_add(1, &v); smp_mb__after_atomic();
```

### 64 位原子操作

```c
// Linux 提供 atomic64_t（64 位原子）
atomic64_t counter = ATOMIC64_INIT(0);
atomic64_inc(&counter);
atomic64_add(100, &counter);
u64 val = atomic64_read(&counter);
atomic64_set(&counter, 0);

// 在 ARM32 上 64 位原子操作用 LDXR/STXR（64 位版本）
// 在 ARM64 上 64 位原子操作直接用 LDXR/STXR（64 位寄存器）
```

## HFT 关联

Linux 原子 API 在用户态 HFT 中不直接可用，但理解其设计有助于正确使用 C++ `std::atomic`。

### C++ atomic 与 Linux API 对照

| Linux API | C++ 等价 | 说明 |
|-----------|---------|------|
| `atomic_read` + `smp_rmb()` | `load(acquire)` | 带可见性保证的读 |
| `atomic_set` + `smp_wmb()` | `store(release)` | 带可见性保证的写 |
| `atomic_add` (relaxed) | `fetch_add(relaxed)` | 无屏障原子加 |
| `atomic_add_return` | `fetch_add(acq_rel)` | 带屏障原子加 |
| `atomic_cmpxchg` + `smp_mb__after_atomic` | `CAS(seq_cst)` | 全序 CAS |
| `smp_load_acquire` | `load(acquire)` | 编译为 LDAR |
| `smp_store_release` | `store(release)` | 编译为 STLR |

关键点：1) `atomic_add`（不带 return）不自带屏障，等价于 C++ `memory_order_relaxed`；2) `atomic_add_return` 自带 acquire/release，等价于 `memory_order_acq_rel`；3) `smp_mb__after_atomic()` 用于在 relaxed 原子操作后补屏障。HFT 中应优先用 `std::atomic` 的内存序参数明确指定语义，而非依赖 Linux API 的隐含行为。

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

4. **HFT 中应该用 Linux 原子 API 还是 C++ std::atomic？为什么？**

<details>
<summary>答案</summary>

HFT 用户态应优先用 **C++ `std::atomic`**。原因：
1. **内存序明确**：`memory_order_acquire/release/relaxed` 语义清晰，不依赖隐含行为
2. **编译器自动优化**：自动选择 LDAR/STLR/LSE 等最优指令
3. **跨平台**：同一份代码在 x86 和 ARM 上正确编译
4. **可维护性**：代码更清晰，不需要记住 API 的隐含屏障规则

Linux API 适合内核开发，但用户态 HFT 用 C++ atomic 更安全。
</details>

## 参考与延伸

- [§20.2 原子操作实现模式](02-atomic-patterns.md) — LDXR/STXR 循环
- [§20.3 ARMv8.1 LSE](03-lse.md) — LDADD 等单指令
- [Ch18 §18.5 Linux 内核屏障 API](../../chapter-18-memory-barriers/notes/section-0-本章完整概述.md) — smp_mb 等详解
