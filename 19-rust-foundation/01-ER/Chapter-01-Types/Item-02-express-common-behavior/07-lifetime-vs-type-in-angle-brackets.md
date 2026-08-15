# Item 2 · 07 尖括号两类参数：生命周期 vs 类型

← [Item 2 目录](./README.md) · 前置：[06 FnOnce 与 `'env`](./06-trait-generic-params.md)

## 先分两类

尖括号 `<>` 里可以同时出现 **生命周期参数 `'a`** 和 **普通类型参数 `T`**，语法、用途、规则不同：

| 写法 | 类别 |
|------|------|
| `'xxx`（带单引号） | **生命周期参数** |
| `i32`、`()`、`String`、`F`（不带引号） | **普通类型参数** |

---

## 1. 只有生命周期参数

### 例：`Read<'buf>`

```rust
trait Read<'buf> {
    fn read(&mut self, buf: &'buf mut [u8]);
}

impl<'buf> Read<'buf> for File {
    fn read(&mut self, buf: &'buf mut [u8]) {}
}
```

`Read<'buf>` 里的 `'buf` 是 **trait 顶层生命周期参数**，约束 trait 内部引用的存活时间。

### 例：`thread::scope`

```rust
thread::scope<'env, F>(closure)
//              ^^^^  ^^
//              |     普通类型参数：闭包类型 F
//              生命周期参数：允许线程借用外部局部变量
```

---

## 2. 生命周期 + 类型参数混写

`FnOnce` 尖括号主体是 **入参元组类型**；元组**内部**可以含带生命周期的引用：

```rust
// 可调用对象：调用时传入 &'a str
fn accept<'a, F: FnOnce((&'a str,))>(f: F) {
    // ...
}
```

拆解 `FnOnce<(&'a str,)>`：

- 整体 `(&'a str,)` 是 **Args 元组**（普通类型参数位置）；
- 元组里的 `'a` 是 **引用类型内部的生命周期标注**，不是 trait 名字后面的顶层参数。

---

## 3. 两种 `'a` 写法，极易混

### 写法 A：trait 顶层 `Trait<'a>`

生命周期写在 **trait 名后面**，传给 trait 本身：

```rust
trait Demo<'a> {}
impl<'a> Demo<'a> for Vec<i32> {}
```

`'a` = **trait 的泛型生命周期参数**，约束整个 impl 层面的借用关系。

### 写法 B：类型内部 `FnOnce<(&'a str,)>`

`'a` 藏在 **入参元组**里，只约束「调用时传入的那根引用」能活多久，**不**是 trait 顶层参数。

---

## 对照表（一眼分清）

| 写法 | 尖括号内容 | 类别 | 作用 |
|------|------------|------|------|
| `FnOnce<()>` | `()` | 类型参数 | 入参列表为空 |
| `FnOnce<(i32,)>` | `(i32,)` | 类型参数 | 入参为单个 `i32` |
| `Read<'buf>` | `'buf` | 生命周期参数 | 约束 trait 内引用 |
| `scope<'env, F>` | `'env` + `F` | 生命周期 + 类型 | `'env` = Scope **环境**；详见 [08](./08-scope-env-lifetime.md) |
| `FnOnce<(&'a str,)>` | 元组内含 `'a` | 类型参数位 + 内部生命周期 | 入参是带生命周期的 `&str` |

---

## 4. 串起来：四条结论

1. **`FnOnce<X>` 的尖括号主体永远是入参元组类型**（不是返回值）。
2. 元组里若出现 `&'a T`，`'a` 只是 **该引用参数** 的生命周期，不是 trait 顶层参数。
3. **`Trait<'a>`** 写在 trait 名后 → **trait 顶层**生命周期（如 `BufRead<'buf>`）。
4. **`Scope<'env>`** 是 **结构体**参数，约束 scope 内子线程对外层环境的借用 → [08](./08-scope-env-lifetime.md)（**不是** trait 内引用那一套）。
5. 元组内 `&'a`：仅约束**调用时传入**的那根引用。

---

## 5. 极简对比示例

```rust
// 1. trait 自身带顶层生命周期 'data
trait HoldData<'data> {
    fn get(&self) -> &'data str;
}

// 2. FnOnce 的参数元组内部带生命周期 's
fn run<'s, F: FnOnce((&'s str,))>(f: F) {
    f(("test",));
}
```

| 位置 | `'data` / `'s` 的角色 |
|------|----------------------|
| `HoldData<'data>` | trait **顶层**生命周期 |
| `FnOnce((&'s str,))` | 仅 **入参引用** 的生命周期 |

---

## 相关

- `FnOnce<()>` 入门 → [06-trait-generic-params.md](./06-trait-generic-params.md)
- 生命周期专题 → [Item 14 生命周期](../Chapter-03-Concepts/Item-14-lifetimes/README.md)
- 借用检查 → [Item 15 借用检查器](../Chapter-03-Concepts/Item-15-borrow-checker/README.md)
- `'env` 与 `Scope` → [08-scope-env-lifetime.md](./08-scope-env-lifetime.md)
