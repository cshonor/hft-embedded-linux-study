# Item 2 · 08 `'env` 到底是什么：`Scope` 不是 trait

← [Item 2 目录](./README.md) · 前置：[06](./06-trait-generic-params.md) · [07](./07-lifetime-vs-type-in-angle-brackets.md)

## 核心结论

`thread::scope` 里的 `'env` **不是**「某个 trait 内部全部引用不能超过 `'env`」。

它约束的是：

> 在这个 scope 里创建的所有子线程，能安全借用的**外层局部变量**，最长存活区间就是 `'env`；  
> 线程里对外层的借用 `&T`，生命周期 **不能超出 `'env`**。

绑定的是 **整个 scope 作用域对应的外层环境**，不是某个 trait 的方法签名。

---

## 底层逻辑

```rust
std::thread::scope(|s| {
    // s: &Scope<'env>
    s.spawn(|| {
        // 可直接借用外层局部变量，不必 move、不必 'static
    });
});
```

| 点 | 说明 |
|----|------|
| `'env` 指什么 | scope **所在外层函数**里局部变量的生命周期 |
| scope 保证什么 | 所有 `spawn` 的线程结束之前，scope 块不会返回 |
| 结果 | 外层 `'env` 变量一定比里面所有线程活得久 → 无悬垂引用 |

---

## 两种生命周期参数，别混

### 情况 A：trait 顶层 `'buf`（你的理解在这里成立）

```rust
trait BufRead<'buf> {
    fn read(&mut self) -> &'buf [u8];
}
```

`'buf` 是 **trait 的泛型生命周期参数**：该 trait 方法里接收/返回的引用，受 `'buf` 约束。

### 情况 B：`Scope<'env>` 的 `'env`（线程 scope 专属）

`Scope` 是 **结构体**，不是 trait：

```rust
pub struct Scope<'env> { /* 标记环境生命周期 */ }
```

`'env` 是 **结构体的生命周期参数**，作用对象是 **依托这个 Scope 生成的子线程**：

1. 子线程闭包里捕获的外层借用，生命周期 ≤ `'env`；
2. 离开 scope 块时 `'env` 结束；编译器 + runtime 保证此时没有线程仍持有该环境的引用。

---

## 对照：trait `'a` vs `Scope<'env>`

| | `Trait<'a>` | `Scope<'env>` |
|--|-------------|---------------|
| 挂在谁上 | trait | **结构体** |
| 约束谁 | 该 trait **方法里的引用** | 该 scope **spawn 的全部线程**对外层环境的借用 |
| 典型例子 | `BufRead<'buf>` | `thread::scope` |

---

## 常见偏差（修正）

1. ❌「`'env` 属于某个 trait」→ ✅ 属于 **`Scope` 结构体**（以及 `scope` 函数引入的环境生命周期）。
2. ❌「约束 trait 内所有引用」→ ✅ 约束 **scope 创建的所有线程**对外层局部变量的借用上限。

---

## 最简示例

```rust
fn demo() {
    let msg = String::from("test"); // 生命周期落在 scope 的 'env 内
    thread::scope(|scope| {
        // scope: &Scope<'env>
        scope.spawn(|| {
            println!("{}", msg); // &msg 生命周期 ≤ 'env，合法
        });
    });
    // 此处：所有子线程已结束，'env 结束，msg 才可安全销毁
}
```

没有 `'env` + scope 的「等线程结束再返回」机制时，`spawn` 通常要求 `'static`。  
正是 `'env` 标出了**外层环境的存活区间**，才允许线程直接借用局部变量。

---

## 相关

- 尖括号里 `'env` 与 `F` 的分工 → [07-lifetime-vs-type-in-angle-brackets.md](./07-lifetime-vs-type-in-angle-brackets.md)
- 借用检查 / 线程 → [Item 15 借用检查器](../Chapter-03-Concepts/Item-15-borrow-checker/README.md)、[Item 17 共享状态](../Chapter-03-Concepts/Item-17-shared-state-parallelism/README.md)
- Book → [16 无畏并发](../../../00-Book/16-fearless-concurrency/)
