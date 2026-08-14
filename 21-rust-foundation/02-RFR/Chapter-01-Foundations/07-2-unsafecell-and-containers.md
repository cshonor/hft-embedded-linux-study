# 3.3.2 · UnsafeCell 与标准库容器速查

> 所属：**Borrowing and Lifetimes · 内部可变性** · [← 07 hub](./07-interior-mutability.md)

← [07.1 外部 vs 内部](./07-1-external-vs-interior.md) · 下一节 [07.3 Cell/RefCell](./07-3-cell-vs-refcell.md)

---

## 一、`UnsafeCell<T>` 底层基石

所有内部可变容器的唯一底层原语 — **opt-out** 普通 `&T` 的「永不修改」假设：

```text
普通 &T     → LLVM 可假定只读（load 消除、寄存器缓存）
UnsafeCell  → 共享路径下也可能写 → 禁止错误优化
```

用户**不直接使用** `UnsafeCell`；`Cell` / `RefCell` / `Mutex` 封装并承担安全契约。

→ Nomicon [五种 unsafe 能力](../../04-Rust-Nomicon/01_Safe_Unsafe/03-five-powers.md) · [05 §LLVM](./05-shared-references.md)

---

## 二、标准库容器速查

| 容器 | 场景 | 读写方式 | 违规 |
|------|------|----------|------|
| `Cell<T>` | 单线程 · **`Copy`** | `get`/`set`，无内部引用 | 类型约束保证安全 |
| `RefCell<T>` | 单线程 · 任意 `T` | `borrow`/`borrow_mut` | **panic** |
| `Mutex<T>` | 多线程写 | `lock()` | 死锁 / **中毒** |
| `RwLock<T>` | 多线程读多写少 | `read`/`write` | 同上 |

→ 单线程详解 [07.3 Cell vs RefCell](./07-3-cell-vs-refcell.md)  
→ 多线程 [RFR 第 10 章](../Chapter-10-Concurrency-and-Parallelism/README.md) · Nomicon [Send/Sync](../../04-Rust-Nomicon/07_Concurrency_Atomic/02-send-sync.md)

---

## 三、Cell vs RefCell：谁有计数器？

| | `Cell` | `RefCell` |
|---|--------|-----------|
| 计数器 | **无** | **有** `BorrowFlag` |
| 互斥手段 | 不产引用，只 `Copy` | 运行时模拟 `&` / `&mut` |

→ 完整原理 [07.3 计数器与 UnsafeCell 内部机制](./07-3-cell-vs-refcell.md)

---

## 四、速记

1. **一切内部可变底层都是 `UnsafeCell`。**  
2. **单线程：`Cell`（Copy）或 `RefCell`（通用）。**  
3. **多线程：放弃 cell，用 `Mutex`/`RwLock`。**

→ 下一节：[07.3 Cell 与 RefCell](./07-3-cell-vs-refcell.md)

---

## 速记

## 子节速记

```text
07.1  外部 mut vs 内部可变 · 为何需要 · 容器路线图
07.2  UnsafeCell opt-out · Cell/RefCell/Mutex 速查 · RefCell 结构
07.3  Cell=无计数器 · get=拷贝/set=原地覆盖 · RefCell=BorrowFlag
07.4  编译期卡写 vs RefCell 运行时校验 · &self/动态生命周期/细粒度
07.5  对比表 · 误区 · 三句话总纲
```

## 三句话

1. **外部**：编译期看 `mut`。  
2. **内部**：外层 `let`，盒内 `Cell`/`RefCell`。  
3. **`RefCell`**：规则同 `&`/`&mut`，但修改代码能编译、冲突运行时 panic。

## 选型

| 场景 | 用 |
|------|-----|
| 默认 | `let mut` + `&mut` |
| 小 Copy + `&self` | `Cell` |
| `String`/Vec + `&self` | `RefCell` |
| `Rc` 共享写 | `Rc<RefCell<T>>` |
| 多线程 | `Mutex` / `RwLock` |

## 自测

- [ ] 画出 `UnsafeCell` 在容器栈中的位置  
- [ ] `Cell` vs `RefCell` 五维对比  
- [ ] `Rc<RefCell<T>>` 解决哪两个问题？

