# CAS 与 Fetch-Modify 专题（第 2～4 章贯通）

> 原书 **第 2 章** API、**第 4 章** CAS 自旋锁、**第 5 章** 通道标志位，都建立在本专题之上。  
> 代码：`id_allocator.rs`、`lazy_init.rs`、`use_atomic_operations.rs`、`spin_lock.rs`（`lock` / `lock_cas`）。

---

## 1. Fetch-and-Modify（获取并修改）

**定义**：一条原子指令内 **读出旧值 + 写入新值**，返回 **修改前的旧值**。

| API | 用途 |
|-----|------|
| `fetch_add` / `fetch_sub` | 计数、ID（注意溢出） |
| `fetch_or` / `fetch_and` | 位图、标志集合 |
| `fetch_max` / `fetch_min` | 统计峰值等 |
| `swap` | 直接替换并返回旧值 |

**溢出**：`fetch_add` / `fetch_sub` 在边界 **静默回绕（wrapping）**，Debug 下普通整数会 panic，原子不会。

---

## 2. Compare-and-Exchange（CAS）

**语义**：若当前值 == **期望值** → 写入 **新值** 并 `Ok(期望)`；否则 **不改** 并 `Err(当前值)`。

### Fetch-Update 循环（必考）

```rust
fn allocate_new_id() -> u32 {
    use std::sync::atomic::{AtomicU32, Ordering::Relaxed};

    static NEXT_ID: AtomicU32 = AtomicU32::new(0);
    let mut id = NEXT_ID.load(Relaxed);
    loop {
        assert!(id < 1000, "too many IDs!");
        match NEXT_ID.compare_exchange_weak(id, id + 1, Relaxed, Relaxed) {
            Ok(_) => return id,
            Err(v) => id = v,
        }
    }
}
```

- **先检查再 CAS**：把「越界判断 + 递增」打成一体，避免 `fetch_add` 后别人继续加导致回绕。  
- 完整版见 **`id_allocator.rs`**。

### `compare_exchange` vs `compare_exchange_weak`

| | strong | weak |
|---|--------|------|
| 虚假失败 | 无 | **可能有**（ARM LL/SC） |
| 循环内 | 可用 | **优先**（失败则重试） |
| 性能 | 略重 | 常更省 |

**规范**：在 **`loop` 里做 CAS → 用 `compare_exchange_weak`**。

---

## 3. CAS 自旋锁（第 4 章）

```rust
while self
    .locked
    .compare_exchange_weak(false, true, Ordering::Acquire, Ordering::Relaxed)
    .is_err()
{
    std::hint::spin_loop();
}
// 解锁：self.locked.store(false, Release);
```

- 与 **`swap(true, Acquire)`**（`spin_lock.rs::lock`）等价语义，缓存争用特征因架构而异。  
- **仅适合极短临界区、多核**；持锁久或单核忙等 → 用 **`Mutex` / futex**。

### CAS 自旋锁优缺点

| 优点 | 缺点 |
|------|------|
| 无 syscall，竞争低时极快 | 竞争高时 **CPU 空转** |
| 用户态完成 | 持锁线程被挂起时，他人仍自旋浪费 |

---

## 4. 与第 5 章 Channel 的关系

- 通道 **`ready`** 用 **Release/Acquire**（不是 Relaxed 信使）。  
- 复杂状态机可再用 **CAS** 扩展；本章 one-shot 用 bool + 内存序即可。  
- 见 [Chapter-05-Channels/本章学习笔记.md](../Chapter-05-Channels/本章学习笔记.md)。

---

## 5. 易错点

1. **`fetch_add` 后 `panic`** — 无法撤销已发生的原子加。  
2. **循环外只用 strong CAS** — 应 weak + loop。  
3. **两个原子变量业务一致** — 分别 fetch 中间态可见 → 用 `Mutex`。  
4. **长临界区 CAS 自旋** — 改阻塞锁或 channel。

---

## 6. 背诵卡

| # | 背这句 |
|---|--------|
| 1 | fetch_* 返回旧值；加减溢出回绕不回 panic。 |
| 2 | CAS：相等才换；失败拿新值；loop 里用 weak。 |
| 3 | ID 防溢出：loop+CAS+先 assert，别 fetch_add 后 panic。 |
| 4 | 自旋锁 CAS false→true Acquire；解锁 Release store false。 |
| 5 | CAS 快但费 CPU；长临界区用 Mutex/park。 |
