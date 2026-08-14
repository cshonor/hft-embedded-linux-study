# Item 1 · 03 聚合类型

← [Item 1 目录](./README.md)

**定义**：把多个值组合成一个整体的类型。

| 类型 | 背一句 |
|------|------|
| **数组** `[T; N]` | **编译期定长**、**同构**；`arr[i]` 下标访问 |
| **元组** `(T1, T2, …)` | 定长、**异构**；`.0`、`.1` 访问 |
| **结构体** `struct` | **命名字段** `s.field` |
| **元组结构体** | 有类型名、无字段名；`.0`、`.1`（约定俗成表达语义） |

---

## 可变性：`mut` 决定，不是「值类型 / 引用类型」

❌ **错误**：值类型默认不可改、引用类型默认可改。  
✅ **正确**：能不能改，看绑定有没有 **`mut`**，和「是不是引用类型」**无直接关系**。

| 声明 | 能否改内容 |
|------|------------|
| `let s = String::from("hello")` | ❌ 不能 `push_str` |
| `let mut s = String::from("hello")` | ✅ 可以改 |
| `let arr = [1, 2, 3]` | ❌ 不能改元素 |
| `let mut arr = [1, 2, 3]` | ✅ 可以 `arr[0] = 0` |

Rust 默认 **`let` 不可变**；要可变必须 **`let mut`**。可变性是**变量/绑定**上的开关，不是类型自带的属性。

### 和 Python 元组的区别

| | Python `tuple` | Rust |
|--|----------------|------|
| 不可变 | **类型级**：元组本身就不能改 | **绑定级**：`let t = (...)` 不可变；`let mut t = (...)` 可变 |
| 灵活性 | 要可变得换 `list` 等另一类型 | **同一类型**（同一 struct / 元组类型）既可 `let` 只读，也可 `let mut` 可写 |

HFT 示例：

```rust
// 不可变：订单初始参数，防止后续误改
let init: OrderParams = load_from_config();

// 可变：同类型或另一实例，存实时中间状态
let mut scratch: OrderParams = init.clone();
scratch.qty = recalc_qty();
```

→ 所有权、`&` / `&mut` 深入见 Book [04 所有权](../../../00-Book/04-ownership/)；这里只记：**默认不可变 + 用 `mut` 打开**。

---

## 怎么访问：数组 vs 元组 / 元组结构体

| 类型 | 访问方式 | 索引特点 |
|------|----------|----------|
| 数组 `[T; N]` | `arr[i]` | `i` 是**运行时**下标，越界 → **panic** |
| 元组 `(T1, T2)` | `t.0`、`t.1` | **编译期**固定字段，从 0 起 |
| 元组结构体 `Color(u8,u8,u8)` | `c.0`、`c.1`、`c.2` | 同元组；`c.3` → **编译错误**（没有第 4 个字段） |

```rust
let t = (1, "a");
assert_eq!(t.0, 1);

struct Color(u8, u8, u8);
let c = Color(255, 0, 128);
assert_eq!(c.0, 255);
// let _ = c.3;  // ❌ 编译期报错，不是运行时越界
```

元组与元组结构体在「点 + 数字」访问上**一致**，学习成本低；和普通 struct 的 **命名字段**（`bar.open`）不同。

---

## 元组结构体：约定俗成的轻量 newtype

把**类型名**和**位置约定**绑在一起，少写字段名仍可读：

```rust
struct Color(u8, u8, u8);   // 约定：.0 .1 .2 = R G B
struct Price(i64);          // 裸 i64 包一层，和 OrderId 不会混
struct OrderId(u64);
```

好处（Effective Rust / newtype 思路的轻量版）：

1. **和裸整数区分**——不会把 `OrderId` 当 `Price` 去加减；
2. **比带字段名的 struct 更省语法**——`Price(p)` 而非 `Price { ticks: p }`；
3. **编译后布局**常与内部单字段类型相同，**无额外运行时开销**。

HFT 里 `Price(i64)`、`Qty(u32)` 这类包装很常见；字段语义复杂时用普通 struct（如下面的 `Bar`）。

---

## 示例：普通 struct 绑 K 线

```rust
struct Point { x: i32, y: i32 }

struct Bar {
    open: f64,
    high: f64,
    low: f64,
    close: f64,
    ts_ms: u64,
}
```

标量描述**单个量**；聚合类型把多个量**绑成一个类型**，布局编译期确定（见 [01 固定大小](./01-fundamental-types.md)）。

> 数组 `arr[i]` 越界在 debug / release 都可能 panic——别当成 C 式静默读错内存。

## 相关

- newtype 深入 → [Item 06 newtype](../Item-06-newtype-pattern/README.md)
- 用 enum 表达状态 → [04-enum-adt.md](./04-enum-adt.md)
- 设计案例 → [06-design-patterns.md](./06-design-patterns.md)
