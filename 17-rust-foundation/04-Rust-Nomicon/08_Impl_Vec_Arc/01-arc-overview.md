# 08 · Example: Implementing Arc · 本章定位

← [本章目录](./README.md) · [Vec 笔记](./00-overview.md) · [全书笔记](../notes.md)

---

官方标题 **Implementing Arc and Mutex**（本节为 **Arc 无弱引用** 简化版）。将布局、内部可变性、原子操作、型变、`PhantomData` 等理论付诸实践。

| 对照 | 路径 |
|------|------|
| Send / Sync / 内存序 | [07_Concurrency_Atomic](../07_Concurrency_Atomic/00-overview.md) |
| forget 与 Rc 溢出 | [06_OBRM_RAII](../06_OBRM_RAII/00-overview.md) |
| PhantomData | [03_Lifetime_Variance](../03_Lifetime_Variance/00-overview.md) |
| Book Arc | [16.3 Mutex/Arc](../../00-Book/16-fearless-concurrency/16.3-共享状态并发.md) |

**读完应能回答**：为何用 `NonNull` + `PhantomData`、Clone 为何 Relaxed、Drop 为何 Release + Acquire fence。

---

## 1. 数据布局 (Layout)

`Arc<T>` 提供堆上 `T` 的**线程安全共享所有权**，引用计数须为 **`AtomicUsize`**，与 `T` 同存于堆结构 **`ArcInner<T>`**。

| 方案 | 问题 |
|------|------|
| `*mut ArcInner<T>` | 失去协变；Drop Check 所有权信息错误 |
| **`NonNull<ArcInner<T>>` + `PhantomData<ArcInner<T>>`** | 非空 + 协变 + 逻辑拥有标记 |

→ 源码：[src/my_arc.rs](./src/my_arc.rs)

---

## 2. 构造 (new) 与解引用 (Deref)

| 步骤 | API |
|------|-----|
| 构造 | `Box::new(ArcInner { rc: 1, data })` → `Box::into_raw` → `NonNull` |
| 访问 | `Deref` → `unsafe { ptr.as_ref() }` → `&T` |

---

## 3. 线程安全标记 (Send / Sync)

```rust
unsafe impl<T: Send + Sync> Send for MyArc<T> {}
unsafe impl<T: Send + Sync> Sync for MyArc<T> {}
```

无 `T: Send + Sync` 边界时，可将 `Rc` 等**非线程安全**类型跨线程传递 → **数据竞争**。

---

## 4. Clone 与防泄漏放大

- **`fetch_add(1, Ordering::Relaxed)`** — Clone 不读写 `T`，无需 happens-before。
- **防 `mem::forget` 溢出**：旧计数 ≥ `isize::MAX` 时 **`process::abort()`**（恶意 forget 克隆可导致 refcount 溢出 → 归零 → UAF）。

---

## 5. Drop 与内存屏障

1. **`fetch_sub(1, Ordering::Release)`**
2. 若返回值 **≠ 1** → 仍有其它引用，直接返回
3. 若 **== 1** → **`atomic::fence(Acquire)`** 再清理
4. **`Box::from_raw(ptr.as_ptr())`** 销毁 `ArcInner`

**Release + Acquire fence**：保证其它线程对 `T` 的业务访问，在时间上先于本线程的物理回收；否则硬件重排可能使 dealloc 早于其它线程读写 → 灾难。

→ 源码：[src/my_arc.rs](./src/my_arc.rs)

---

← 上一节：[08-zst.md](./08-zst.md) · 下一节：