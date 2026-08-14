# Item 2 · 06 Trait 尖括号：`FnOnce<()>` vs `'env`

← [Item 2 目录](./README.md)

逐层拆解两个常混点：

1. `Trait<Args>` 这种带尖括号的 trait，和 `Display` 等无参 trait **实现语法同一套**；
2. `FnOnce<()>` 里尖括号的 `()` 是什么、为什么能「把参数传进 trait」；
3. 对比 `thread::scope` 的 `'env`：**trait 类型参数** vs **生命周期参数**——写法相似，用途完全不同。

---

## 一、共识：trait 后 `<>` 里可以放什么

所有 trait 都支持在名字后写 `<…>`，常见两类：

| 种类 | 例子 | 管什么 |
|------|------|--------|
| **类型参数** | `FnOnce<()>`、`Iterator<Item = i32>` | 配套的类型（含元组） |
| **生命周期参数** | `impl<'a> …`、`scope<'env, F>` | 借用能活多久 |

实现语法统一：

```rust
impl Display for MyStruct {}

impl FnOnce<()> for MyFunc {}

impl<'a> BufRead<'a> for File {}  // trait 顶层生命周期，见 07 / 08
```

尖括号本质：**给 trait 传静态配置**，让编译器在编译期确定本次实现对应的类型 / 生命周期。和 `Vec<T>` 的泛型逻辑一致。

---

## 二、`FnOnce<Args>` 里的 `()` 是什么

简化标准库形状：

```rust
trait FnOnce<Args> {
    type Output;
    // Args：调用该可调用对象时的**参数列表**（元组）
    fn call_once(self, args: Args) -> Self::Output;
}
```

### 1. `Args` 是元组，承载入参

| 调用签名 | `Args` | trait 写法 |
|----------|--------|------------|
| 无参 `fn()` | `()` | `FnOnce<()>` |
| 单参 `fn(i32)` | `(i32,)` | `FnOnce<(i32,)>` |
| 双参 `fn(i32, String)` | `(i32, String)` | `FnOnce<(i32, String)>` |

### 2. 为什么把「调用参数」放进 trait 泛型？

Rust trait 静态分发，编译期必须确定：

1. 调用时要传什么类型的参数；
2. 返回什么类型（关联类型 `Output`）；
3. `call_once` 的 `args` 容器类型就是 `Args`。

因此 **`<()>` 不是「返回空」**，而是 **「调用时入参为空（空元组）」**。

别混两个 `()`：

| 写法 | 含义 |
|------|------|
| `FnOnce<()>` | 尖括号：入参列表为空 |
| `-> ()` | 箭头：函数返回单元类型 |

```rust
// 无入参、返回 i32：|| 10
// 概念上：FnOnce<(), Output = i32>

// 入参 i32、无返回值：|x| { let _ = x; }
// 概念上：FnOnce<(i32,), Output = ()>
```

### 3. `impl FnOnce<()> for MyFunc` 的含义

为 `MyFunc` 实现 **无入参版** `FnOnce`：

- 调用 `my_func()` 时不传参；
- `call_once(self, args: ())` 里 `args` 是空元组，没有数据。

---

## 三、对比 `scope` 的 `'env`（概要）

`thread::scope` 里 `'env` 绑定的是 **Scope 结构体 + 外层局部环境**，不是 trait 方法里的引用约束。

- `'env`：scope 内外层变量的存活区间；子线程借用不能超过它。
- `()` / `(T,)`：FnOnce 的**入参元组**（与 `'env` 无关）。

→ **精准说明**（Scope 不是 trait、与 `BufRead<'buf>` 的差异）：[08-scope-env-lifetime.md](./08-scope-env-lifetime.md)

---

## 四、完整小例子

```rust
struct FuncObj;

impl FnOnce<()> for FuncObj {
    type Output = i32;
    fn call_once(self, _args: ()) -> i32 {
        666
    }
}

struct FuncObj2;

impl FnOnce<(i32,)> for FuncObj2 {
    type Output = String;
    fn call_once(self, args: (i32,)) -> String {
        args.0.to_string()
    }
}
```

| 类型 | 对应闭包直觉 |
|------|----------------|
| `FuncObj` | `\|\| 666`（无参） |
| `FuncObj2` | `\|x\| x.to_string()`（一个 `i32`） |

> 日常闭包由编译器自动实现 `Fn*`；手写 `impl FnOnce` 多用于理解 trait 形状或特殊封装。

---

## 五、核心疑问速答

1. **为什么 `impl Trait` 后 `<>` 里能写东西？**  
   Trait 支持泛型参数，尖括号补充静态信息，与 `Vec<T>` 同理。

2. **`FnOnce<()>` 的 `()` 是什么？**  
   调用时的**入参为空**，不是返回值。

3. **和 `'env` 的区别？**  
   `'env` = 生命周期；`()` = 入参元组类型。

4. **和 `Display` 实现一致吗？**  
   语法同一套；`Display` 无泛型参数，`FnOnce` 必须指定 `Args`（及 `Output`）。

## 相关

- 尖括号两类参数（顶层 `'a` vs 元组内 `&'a`）→ [07-lifetime-vs-type-in-angle-brackets.md](./07-lifetime-vs-type-in-angle-brackets.md)
- **`'env` / Scope** → [08-scope-env-lifetime.md](./08-scope-env-lifetime.md)
- 闭包 trait 概览 → [01-core-concepts.md](./01-core-concepts.md)
- API 对比示例 → [04-examples.md](./04-examples.md)
- 单态化 vs trait 对象 → [Item 12](../Chapter-02-Traits/Item-12-generics-vs-trait-objects/README.md)
