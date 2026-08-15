# Item 2 · 重点结论

← [Item 2 目录](./README.md)

Rust **回调与泛型设计**四条工程实践（Effective Rust Item 2 核心）。

---

## 1. 回调 API 优先 `Fn*`，少用裸 `fn`

**让调用方能捕获环境。**

| | 裸 `fn` | `Fn` / `FnMut` / `FnOnce` |
|---|---------|----------------------------|
| 本质 | 函数指针 | trait，闭包自动实现 |
| 捕获环境 | ❌ 不能 | ✅ 可以 |
| 典型用途 | 全局/静态函数、C 回调 | Rust 回调、迭代器、策略注入 |

```rust
let factor = 2;

// ❌ 闭包无法传给 fn 指针
// fn apply(f: fn(i32) -> i32, x: i32) -> i32 { f(x) }
// apply(|x| x * factor, 10); // 编译错误：不能捕获 factor

// ✅ 泛型 + FnOnce：调用方可捕获
fn apply<F: FnOnce(i32) -> i32>(f: F, x: i32) -> i32 { f(x) }
apply(|x| x * factor, 10); // OK
```

→ 更多对比 → [04-examples.md](./04-examples.md)

---

## 2. 参数用「能干活的最宽」`Fn*`

**只调用一次就写 `FnOnce`，别无谓收紧为 `Fn`。**

继承：`Fn: FnMut: FnOnce`（闭包**能力** Fn 最强；API **约束** FnOnce 最宽）。

| 你怎么用回调 | 参数应写 | 原因 |
|--------------|----------|------|
| 只调**一次** | **`FnOnce`** | 接受 `move`、消耗环境的闭包 |
| 调多次且**改**捕获 | `FnMut` | 需要 `&mut` 环境 |
| 调多次且**只读** | `Fn` | 最窄，仅只读闭包 |

**原则：在「能正确编译运行」前提下，选最宽的 bound。**

```text
能 FnOnce 满足  →  别写 FnMut
能 FnMut 满足   →  别写 Fn
```

```rust
fn run_once<F: FnOnce()>(f: F) { f(); }   // ✅ 推荐：最宽
fn run_read<F: Fn()>(f: F) { f(); f(); }  // 仅当必须多次只读调用
```

→ 兼容表与完整示例 → [02-logic-flow.md](./02-logic-flow.md) §二

---

## 3. 参数优先 Trait bound，少绑死具体 struct

**便于扩展与测试。**

```rust
use std::io::Read;

// ❌ 绑死实现
// fn load_from_file(f: &mut File) -> std::io::Result<Vec<u8>> { ... }

// ✅ 任何可读源都行：File、TcpStream、Cursor、测试 mock
fn load<R: Read>(r: &mut R) -> std::io::Result<Vec<u8>> {
    let mut buf = Vec::new();
    r.read_to_end(&mut buf)?;
    Ok(buf)
}
```

好处：

- 同一函数服务**多种类型**（文件、网络、内存 buffer）
- 单元测试可传 **mock / `Cursor<&[u8]>`**，不必真开文件

---

## 4. 无 Trait bound 的泛型 `T` 几乎啥也不能干

编译器对裸 `T` 只知道：能 **move**、能 **drop**。

| 裸 `T` 不能做 | 加了 bound 才能 |
|---------------|-----------------|
| `x == y` | `T: PartialEq` |
| `println!("{x}")` | `T: Display` |
| `x.clone()` | `T: Clone` |
| 调用自定义方法 | `T: YourTrait` |

```rust
// ❌ 裸 T：什么方法都调不了
fn broken<T>(x: T) {
    // println!("{x}");     // 需要 Display
    // let _ = x == x;      // 需要 PartialEq
    drop(x);                // ✅ move/drop 可以
}

// ✅ 声明需要什么能力
fn show<T: std::fmt::Display + Clone>(x: T) {
    let y = x.clone();
    println!("{y}");
}
```

C++ 模板有时「有同名方法就能用」；Rust **必须在签名处显式 trait bound**，契约清晰、错误前置到编译期。

→ C++ 对比 → [05-pitfalls.md](./05-pitfalls.md)

---

## 四句话速记

1. 回调用 **`Fn*`**，别用裸 **`fn`**
2. 只调一次 → **`FnOnce`**；约束越宽越好
3. 参数写 **`T: Trait`**，别写死 **`File`**
4. 裸 **`T`** 只能 move/drop；要干活就加 **bound**

## 相关

- **可运行 demo** → [demo/](./demo/)（`cargo run -p item-02-callbacks-generics`）
- 逻辑脉络 → [02-logic-flow.md](./02-logic-flow.md)
- 案例 → [04-examples.md](./04-examples.md)
- 闭包 `FnOnce<()>` → [06-trait-generic-params.md](./06-trait-generic-params.md)
