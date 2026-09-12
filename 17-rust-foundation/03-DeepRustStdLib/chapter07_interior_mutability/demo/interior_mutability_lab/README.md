# 第 7 章 · 内部可变性 —— 动手 Lab

> 上：[chapter07_interior_mutability](../README.md) · 笔记：[7.2.1 UnsafeCell](../7.2.1-unsafecell.md) · [7.2.2 Cell](../7.2.2-cell.md) · [7.3 RefCell](../7.3-refcell-overview.md)

## 跑起来

```bash
cargo test --manifest-path 17-rust-foundation/03-DeepRustStdLib/chapter07_interior_mutability/demo/interior_mutability_lab/Cargo.toml
```

10 个测试，零 error 零 warning。

## 实现了什么

| 类型 | 对应 std | 一句话 |
|---|---|---|
| `MiniCell<T>` | `Cell<T>` | 只能整体搬进搬出，因此**永不产生别名** |
| `MiniRefCell<T>` | `RefCell<T>` | 用 `isize` 计数把借用检查从编译期挪到运行期 |
| `MiniRef` / `MiniRefMut` | `Ref` / `RefMut` | RAII guard，`Drop` 里把计数减回去 |

## 三个容易被忽略的点

### 1. `UnsafeCell` 才是那个唯一的洞

`MiniCell` / `MiniRefCell` 里**没有**写任何 `unsafe impl Sync`，
它们却自动是 `!Sync` —— 因为内含 `UnsafeCell<T>`，而 `UnsafeCell` 在 std 里是 `!Sync`。
「含 `!Sync` 字段 ⇒ 自身 `!Sync`」由编译器自动推导，**不需要** `negative impl`（稳定版也拿不到）。

这就是"内部可变性"的全部地基：标准库只开了一个 `UnsafeCell` 的口子，
其余全靠类型系统往外长。

### 2. `set` 的顺序：先写入，再析构旧值

```rust
pub fn set(&self, value: T) {
    let old = self.replace(value);  // 新值已进槽位
    drop(old);                      // 旧值此刻才析构
}
```

顺序反过来的话，旧值的 `Drop::drop` 里若再次访问本 Cell（**重入**），
就会看到一个"旧值已走、新值未到"的中间态。
测试 `cell_old_value_sees_new_value_during_drop` 专门钉住这条语义。

### 3. 借用计数只有三种状态

| flag | 含义 |
|:--:|---|
| `0` | 无人借用 |
| `n > 0` | n 个共享借用（可叠加） |
| `-1` | 1 个独占借用（拒绝一切其他借用） |

`try_borrow` 只拒绝 `flag < 0`；`try_borrow_mut` 要求 `flag == 0`。
两个 guard 的 `Drop` 分别做 `flag - 1` 和 `flag = 0`。

## 与 std 的差异（刻意为之）

- 不支持 `T: ?Sized` 上的 `new`（`MiniRefCell` 结构体本身支持 `?Sized`）
- 没有 `Ref::map` / `RefMut::map` 这类拆分借用
- `borrow()` 的 panic 信息是中文，方便定位

## 下一步

第 11 章的每个锁，内部数据字段都是 `UnsafeCell` —— 见
[chapter11 的 concurrency_lab](../../chapter11_concurrency/demo/concurrency_lab/README.md)。
