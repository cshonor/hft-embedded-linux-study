# 04.2 · Move / Copy / Clone（一次讲透）

← [04 所有权索引](./04-ownership.md) · 上一节：[04-1-three-rules.md](./04-1-three-rules.md) · 下一节：[04-3-drop.md](./04-3-drop.md)

---

## 核心：不是「堆 Move、栈 Copy」，而是 **`Copy` trait**

Rust **不按栈/堆**决定 Move 还是 Copy，而看**类型是否实现 `Copy`**：

| | **实现了 `Copy`** | **未实现 `Copy`（默认）** |
|---|-------------------|---------------------------|
| **赋值 / 传参** | 自动**按位复制**栈上表示；**原绑定仍有效** | **Move**：所有权转移；**原绑定失效** |
| **堆上 payload** | Copy 类型通常**没有**需 drop 的堆（或整值在栈） | Move **不复制堆数据**，只转移所有权 |

**反例 — 栈上也可以 Move**：

```rust
let a = Box::new(10);  // 句柄在栈，数据在堆；Box 无 Copy
let b = a;             // Move，a 失效

struct Point { x: i32, y: i32 }  // 全在栈，默认仍无 Copy
let p1 = Point { x: 1, y: 2 };
let p2 = p1;           // Move，p1 失效
```

**为何有「栈 Copy、堆 Move」的错觉？**  
大部分**小标量**默认 `Copy`（在栈）；`String` / `Vec` / `Box` 等**带堆或自定义 Drop** 的类型默认**不** `Copy` — 相关，但不是因果。

---

## 典型 Copy 类型

- 整数 / 浮点：`i32`、`u64`、`f64` …
- `bool`、`char`
- `[T; N]`（`T: Copy` 时）
- `(T1, T2, …)`（各元素均 `Copy` 时）
- **`&T`**（共享引用是 Copy）
- **不是 Copy**：`&mut T`、`String`、`Vec`、`Box<T>`、多数自定义 struct/enum

```rust
let a: i32 = 10;
let b = a;              // Copy：复制 4 字节，a、b 都有效
println!("{a} {b}");
```

---

## Move 的本质：转移所有权，不是复制堆

`let s2 = s1;`（`String`）时：

1. **栈上** `{ ptr, len, cap }` **按位复制**一份给 `s2`（机器层面像 memcpy）
2. **堆上**字节**只有一份** — 所有权从 `s1` 转到 `s2`，**不**做堆深拷贝
3. 编译器标记 **`s1` 失效**，不能再访问
4. 作用域结束时只有 **`s2`** 负责 `Drop` 堆

```rust
let s1 = String::from("hello");
let s2 = s1;   // Move
// println!("{s1}"); // ❌
```

→ 需要**两份堆数据**时用 **`clone()`**，不是赋值。

---

## Copy vs Clone

| | **Copy** | **Clone** |
|---|----------|-----------|
| **何时发生** | 赋值、传参时**隐式** | 必须**显式** `.clone()` |
| **语义** | 按位复制**栈上表示**（浅拷贝） | 可**自定义**；`String::clone()` 会**复制堆** |
| **开销** | 极小（几个字节） | 可能很大（整段堆分配 + 拷贝） |
| **实现** | `Copy` + `Clone`（通常 `derive`）；**不能**自定义 Copy 逻辑 | `impl Clone` 可手写 |
| **与 Drop** | 有自定义 **`Drop` 则不能 `Copy`** | 与 Drop 可共存 |

```rust
let a = 10;
let b = a;                    // Copy

let s1 = String::from("hello");
let s2 = s1.clone();          // Clone：堆上完整复制一份
println!("{s1} {s2}");         // 两者都有效

let s3 = s1;                    // Move（若写这句则 s1 失效）
```

**记忆**：**Copy = 隐式、便宜、原变量还在；Clone = 显式、可能贵、两份独立数据。**

---

## 一句话

> **Move / Copy 由 `Copy` trait 决定，不由栈/堆决定：**  
> 有 `Copy` → 赋值时自动按位复制，原绑定有效；  
> 无 `Copy` → Move，原绑定失效，堆数据不复制、只转移所有权；  
> 要 duplicate 堆内容 → **`.clone()`**。

```rust
let v = vec![1, 2];
let w = v;           // Move
// let u = v.clone(); // 若先要 v 再用，先 clone 再分别 move

let x = 5;
let y = x;           // Copy
println!("{x} {y}");
```

---

## 速记

## 三句背诵

1. **三条规则：唯一所有者 · 出作用域 drop · 非 Copy 则 move。**
2. **Move/Copy 看 `Copy` trait，不看栈/堆；要两份堆 → `.clone()`。**
3. **局部变量 drop 逆声明；struct 字段 drop 正定义 — 方向相反。**

## 自测

- [ ] 能解释栈上 `Point` 为何默认仍是 Move
- [ ] 能说出 `String` move 时堆上只有一份数据
- [ ] 能背出局部变量 vs struct 字段的 drop 顺序
- [ ] 能说明 `&T` 为何不 drop 目标
- [ ] 能区分 unwind 与 `panic=abort` 下的 Drop

## 术语表

| 术语 | 含义 |
|------|------|
| RAII | 获取资源即初始化；出作用域自动释放 |
| Move | 转移所有权；原绑定失效 |
| Copy | 隐式按位复制；原绑定仍有效 |
| Clone | 显式复制；可深拷贝堆 |

