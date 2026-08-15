# 1.3.2 · `?Sized` 与 `?` — 否定默认泛型约束

← [03-1 Blanket 完整解读](./03-1-ergonomic-blanket-full-guide.md)

---

## 核心：`?` = 关掉编译器自动套上的约束

Rust 里每个**未写 bound 的泛型类型参数** `T`，编译器**隐式**加上：

```rust
T: Sized
```

写 **`T: ?Sized`** 不是「新增一条要求」，而是 **否定这条默认规则** — 变成「`T` 可以是 `Sized`，也可以不是（DST）」。

### 只有 `Sized` 能用 `?` —— `Send` / `Sync` 不行

| Trait | 泛型 `T` 上是否**隐式**带上 | 能否写 `?Trait` 取消 |
|-------|:--------------------------:|:--------------------:|
| **`Sized`** | ✅ 默认就有 `T: Sized` | ✅ **`T: ?Sized`** |
| **`Send` / `Sync` / `Debug`…** | ❌ 不会自动加 | ❌ **没有** `?Send` 这种用法 |

```rust
fn foo<T>() { }           // 编译器视同为 foo<T: Sized>()
fn bar<T: ?Sized>() { }  // ✅ 合法：取消隐式 Sized

// fn baz<T: ?Send>() { }  // ❌ 不存在：Send 本来就不是默认 bound
// 要 Send 须显式写：fn qux<T: Send>() { }
```

**原因**：`?` 只能「关掉」**本来就有、你没写却生效**的隐式约束。目前泛型类型参数上**唯一**这种隐式约束就是 **`Sized`**；`Send`、`Sync` 等都要你**自己写出来**才生效，没有「默认加了要关」一说，自然也用不了 `?`。

| 写法 | 对 `T` 的含义 |
|------|---------------|
| `T`（默认） | 必须是 **Sized**（编译期已知大小） |
| `T: ?Sized` | **允许** DST：`str`、`[u8]`、`dyn Trait`… |
| `T: Sized` | 显式再强调必须 Sized（与默认等价） |

---

## 和三类公开 Trait **无关**

| 误解 | 正解 |
|------|------|
| `?` 针对「几乎总实现的 Debug / Send…」 | ❌ 与 §02 三类 trait **无关** |
| `?` 关闭「官方默认 trait 实现」 | ❌ 不是 ER Item 13 的 default method |
| `?Sized` 只管 **泛型参数 `T` 的隐式 `Sized`** | ✅ 自己写的 blanket 与 std 一样适用 |

---

## 为何 blanket 里要写 `?Sized`

约束绑在 **泛型参数 `T` 上**，不是绑在 **`&T` 这个包装类型**上。

```rust
impl<T: MyTrait + ?Sized> MyTrait for &T { /* … */ }
//   ^ 这里约束的是 T        ^ impl 的目标类型是 &T
```

### 反例：不加 `?Sized`

```rust
// 隐式 T: Sized
impl<T: MyTrait> MyTrait for &T { /* … */ }

let s: &str = "hi";
// 想让 &str 走 blanket：T = str，但 str 是 DST、!Sized → impl 不适用
```

`&str` 本身是 fat pointer（**Sized**），但被 impl 的 pattern 是 `&T`，其中 **`T = str` 不满足默认 `Sized`** → 编译器拒绝匹配这条 blanket。

加 **`?Sized`** 后：`T` 可以是 `str`，`&str` 就能命中 `impl for &T`。

```rust
impl<T: MyTrait + ?Sized> MyTrait for &T { /* … */ }
// 现在 T = str、T = [u8]、T = dyn MyTrait 均可（若内层类型 impl 了 MyTrait）
```

```text
检查 impl 是否适用时：
  看 T 满不满足 where / 默认 bound  →  不是看 &T 有多大
```

---

## 与 `Sized` trait 本身

| 类型 | `Sized?` |
|------|----------|
| `i32`、`Foo` | ✅ Sized |
| `str`、`[u8]`、`dyn Debug` | ❌ **!Sized**（DST） |
| `&str`、`Box<dyn Trait>` | ✅ 指针/包装本身 Sized |

**`?Sized`**：允许 `T` 落在右列；**包装类型**（`&T`、`Box<T>`）通常仍是 Sized 的 fat/thin pointer。

---

## 速记

```text
?Sized  =  取消「T 必须 Sized」的默认帽子（仅此一例）
?Send   =  不存在 — Send 非隐式 bound
用途    =  blanket for &T / Box<T> 时兼容 str、切片、trait 对象
无关    =  Debug/Send/Copy 三类 · default method · 公开/私有 trait
```

→ 回 [03-1 §四 包装转发模板](./03-1-ergonomic-blanket-full-guide.md#四形态-b-详解包装转发模板)
