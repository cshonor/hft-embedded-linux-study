# 1.1.2.1 · `into_inner` 拆解包装（与 `into_item` 辨析）

← [01-2 `into_`](./01-2-into-series.md) · [01-2-1 详解](./01-2-1-into-inner.md)

Demo → `cargo run --manifest-path naming-series-demo/Cargo.toml into-inner`

---

## 核心逻辑

**`into_inner`**：消耗**包装类型本身**，取出内部 **`T` 的所有权** — 「拆壳拿内容」。

| 点 | 说明 |
|----|------|
| **消耗谁** | 包装器（`Mutex`、`RefCell`、自定义 newtype…），不是「借用的守卫」 |
| **返回值** | 内部 owned 值 `T` |
| **与 `as_`** | `as_ref` 只借；`into_inner` 交壳 |
| **与 `into_iter`** | `into_iter` 拆**容器**变迭代器；`into_inner` 拆**单层包装**拿 `T` |

```text
Mutex<T> ──into_inner(self)──► T          （消耗 Mutex）
RefCell<T> ──into_inner(self)──► T
自定义 Encrypted<T> ──into_inner(self)──► T   （解密后交出）
```

---

## 可运行示例 1：`Mutex::into_inner`（在 Mutex 上，不在 Guard 上）

⚠️ **`MutexGuard` 没有 `into_inner()`** — guard 只是临时借用，通过 `Deref`/`DerefMut` 访问 `&T` / `&mut T`。

要**拿走内部所有权**，须**拥有 `Mutex` 本身**并消耗它：

```rust
use std::sync::Mutex;

let lock = Mutex::new(String::from("hello"));
// guard.into_inner()  // ❌ MutexGuard 无此方法
let inner_str = lock.into_inner().unwrap(); // ✅ 消耗 Mutex → owned String
assert_eq!(inner_str, "hello");
```

若已持有 guard、仍要 owned 值 → **`clone()`** 或 **`std::mem::take`**（`T: Default`）等，不能靠 `into_inner` 从 guard 抽走。

---

## 可运行示例 2：`RefCell::into_inner`

```rust
use std::cell::RefCell;

let cell = RefCell::new(String::from("hi"));
let s = cell.into_inner();
assert_eq!(s, "hi");
```

---

## 可运行示例 3：自定义包装器

库作者常给 newtype / 包装 struct 实现 `into_inner`，语义与 std 一致：

```rust
struct Encrypted(String);

impl Encrypted {
    fn into_inner(self) -> String {
        // 解密逻辑…
        self.0
    }
}

let e = Encrypted("cipher".into());
let plain = e.into_inner();
```

→ [04 Wrapper Types](./04-wrapper-types.md) · ER [Item 06 newtype](../../01-ER/Chapter-01-Types/Item-06-newtype-pattern/README.md)

---

## std 里常见的 `into_inner`（速查）

| 类型 | 签名要点 |
|------|----------|
| **`Mutex<T>`** | `into_inner(self) -> LockResult<T>` |
| **`RwLock<T>`** | 同上 |
| **`RefCell<T>`** | `into_inner(self) -> T` |
| **`Cell<T>`** | `into_inner(self) -> T`（`T: Copy`） |
| **`PoisonError<T>`** | 取出被 poison 前的值 |
| **`Box<T>`** | `Box::into_inner`（部分版本仍不稳定；stable 可用 `*b` 解引用 move） |

---

## 关于 `into_item` — std **没有**这个惯例

标准库 **`Peekable` 没有 `into_item()`**。迭代器产出元素的标准 API 是：

| 需求 | std API |
|------|---------|
| 取下一个 **owned** 元素 | **`next()`** → `Option<Item>` |
| `peek` 后再取走该元素 | **`peek()` 然后 `next()`** — 会消费 peek 缓存 |
| 拆掉 `Peekable` 适配器 | **`into_iter()`** on `Peekable` → 返回**内层迭代器 `I`**，不是单个 `Item` |

```rust
let mut iter = vec![1, 2, 3].into_iter().peekable();
assert_eq!(iter.peek(), Some(&1));
// iter.into_item()  // ❌ std 无此方法
let item = iter.next().unwrap(); // ✅ peek 过的 1 由 next 取出
assert_eq!(item, 1);
```

**`Iterator::Item`** 是关联**类型名**（每个元素类型叫什么），不是 `into_item()` 方法。

若第三方库定义 `into_item`，属于**自定义命名** — 调用前看文档；不要假设所有迭代器都有。

---

## 规律小结

| 方法 | 拆什么 | 得到什么 |
|------|--------|----------|
| **`into_inner`** | 包装层（锁、Cell、自定义壳） | 内部 **`T`** 所有权 |
| **`into_iter`** | 容器 / 可迭代物 | **`Iterator<Item = T>`** |
| **`next`** | 不消耗迭代器本身 | 单个 **`Item`**（可选） |

`Vec`、`String` 等容器通常**没有** `into_inner` — 它们不是「包一层 T」的 wrapper，而是直接拥有数据；拿走内容用 `into_iter`、`into_bytes`、`into_boxed_slice` 等**更具体**的 `into_*`。

---

→ 回 [01-2 `into_`](./01-2-into-series.md)
