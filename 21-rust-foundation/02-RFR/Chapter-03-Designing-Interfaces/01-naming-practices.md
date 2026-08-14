# 1.1 Naming Practices（命名惯例）

> 所属：**Unsurprising** · [← 章索引](./README.md)

← [章索引](./README.md) · 下一节 [02 通用 Trait](./02-common-traits-for-types.md)

Book → [7.2 引用项命名](../../00-Book/07-packages-modules/7.2-引用项命名.md)

---

优秀接口应符合社区直觉：用户「盲猜」用法时也应大概率猜对。

**原则**：复用 std / 生态约定俗成的名字；方法名应反映**是否消耗 `self`**、**返回借用还是拥有值**。

---

## 五系列子节（01-1 ~ 01-5）

| 编号 | 系列 | 核心 | 详例 |
|------|------|------|------|
| **01-1** | **`as_`** | 只读借用，不消耗 | [01-1-as-series](./01-1-as-series.md) |
| **01-2** | **`into_`** | 所有权转移 | [01-2-into-series](./01-2-into-series.md) |
| **01-3** | **`get_`** | 安全访问，`Option` | [01-3-get-series](./01-3-get-series.md) |
| **01-4** | **`try_`** | 错误处理，`Result` | [01-4-try-series](./01-4-try-series.md) |
| **01-5** | **`with_`** | 建造者 / 构造配置 | [01-5-with-series](./01-5-with-series.md) |

Demo → [`naming-series-demo/`](./naming-series-demo/) · `cargo run --manifest-path naming-series-demo/Cargo.toml [as|into|get|try|with]`

---

## 一、`as_` 系列 — 不拿所有权，只转引用

| 方法 | 作用 |
|------|------|
| **`as_ref()`** | `Option<T>` → `Option<&T>`；`Result<T,E>` → `Result<&T,&E>` |
| **`as_mut()`** | 可变版 |
| **`as_ptr()`** | 转裸指针 — unsafe |

→ [01-1 详例 + demo](./01-1-as-series.md) · **口诀**：`as` 只借、不消耗本体。

---

## 二、`into_` 系列 — 拿走所有权，消耗原变量

| 方法 | 典型 |
|------|------|
| **`into_inner()`** | 包装类型 → 内部值 |
| **`into_iter()`** | 消耗容器 → owned 迭代器 |
| **`into_string()` / `into_vec()`** | 类型转换 |

→ [01-2 详例 + demo](./01-2-into-series.md) · [04 包装类型](./04-wrapper-types.md) · **口诀**：`into` = 交出自己。

---

## 三、`get_` 系列 — 安全借用，不 panic

| 方法 | 说明 |
|------|------|
| **`get(index)`** | 越界 → `None` |
| **`get_mut(index)`** | 可变引用版 |

→ [01-3 详例 + demo](./01-3-get-series.md)

---

## 四、`try_` 系列 — 可能失败，返回 `Result`

| 方法 | 说明 |
|------|------|
| **`try_into()`** | 带错误的转换 |
| **`try_lock()`** | 锁拿不到 → `Err`，不阻塞 |

→ [01-4 详例 + demo](./01-4-try-series.md)

---

## 五、`with_` 系列 — 构造 / 配置

| 方法 | 说明 |
|------|------|
| **`Vec::with_capacity(n)`** | 预分配容量 |
| **`Builder::with_xxx()`** | 链式配置 |

→ [01-5 详例 + demo](./01-5-with-series.md)

---

## 六、迭代器三巨头：`iter` / `iter_mut` / `into_iter`

| 方法 | 元素类型 | 容器所有权 | 用途 |
|------|----------|------------|------|
| **`iter()`** | `&T` | **不消耗** | 只读遍历 |
| **`iter_mut()`** | `&mut T` | **不消耗** | 遍历中修改 |
| **`into_iter()`** | `T` | **消耗**容器 | 拿走每个元素 |

### `for` 循环自动选择

```rust
for x in v {}           // 等价 v.into_iter() — 消耗 v
for x in &v {}          // 等价 v.iter()
for x in &mut v {}      // 等价 v.iter_mut()
```

→ 借用 vs 拥有：[07 Borrowed vs Owned](./07-borrowed-vs-owned.md)

---

## 七、整体速记对照表

| 前缀 / 方法 | 所有权 | 返回形式 |
|-------------|--------|----------|
| `as_ref` / `as_mut` | 借用，不消耗 | 包装后的引用 |
| `get` / `get_mut` | 借用，安全 | `Option` 引用 |
| `into_*` | 转移所有权 | 内部原值 / owned 迭代 |
| `try_*` | 通常不消耗 self | `Result` |
| `with_*` | 新建 | 配置完成的对象 |
| `iter` | 不可变借用 | `Item = &T` |
| `iter_mut` | 可变借用 | `Item = &mut T` |
| `into_iter` | 消耗容器 | `Item = T` |

---

## 八、反例（破坏直觉）

| 反例 | 问题 |
|------|------|
| `iter()` 却消耗所有权 | 与 std 语义冲突 |
| `get()` 与 `get_mut()` 不对称且无文档 | 增加挫败感 |
| 该 `try_` 却 panic | 调用方无法处理失败 |

---

## 九、速记

| 前缀 | 口诀 |
|------|------|
| `as_` | 只借，不消耗 |
| `into_` | 交出自己 |
| `get_` | 安全 `Option` |
| `try_` | 失败 `Result` |
| `with_` | 构造 / 配置 |

**迭代器三巨头**：`iter` → `&T` 保留 · `iter_mut` → `&mut T` · `into_iter` → `T` 消耗

**for**：`for x in v` → `into_iter` · `&v` → `iter` · `&mut v` → `iter_mut`

**自测**：五个系列各自消耗还是借用 `self`？ · `get(99)` 和 `[99]` 有何不同？

---

→ 下一节：[02 通用 Trait](./02-common-traits-for-types.md)
