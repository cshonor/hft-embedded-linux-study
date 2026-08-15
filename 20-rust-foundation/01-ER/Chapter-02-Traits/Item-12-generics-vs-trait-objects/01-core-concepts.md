# Item 12 · 核心知识点

← [Item 12 目录](./README.md)

### 泛型 + Trait Bound

```rust
fn draw<T: Draw>(x: &T) { ... }
// 或 where T: Debug + Draw
```

- 编译期 **单态化（Monomorphization）**：每种具体 `T` 一份机器码 → **静态分发**。
- 代价：编译更慢、二进制可能**膨胀**；收益：可内联、无 vtable 间接跳转。

### Trait 对象 `dyn Trait`

```rust
let shapes: Vec<&dyn Draw> = vec![&circle, &square];
```

- **胖指针**：数据指针 + **vtable** 指针 → **动态分发**。
- 形式：`&dyn Trait`、`Box<dyn Trait>` 等。

### 对象安全（Object Safety）

Trait 要能做成 `dyn Trait`，通常需满足（简化记忆）：

1. 方法不能是**泛型方法**（无 `<T>` 等方法级泛型）。
2. 除 `self` 接收者外，签名里不能出现 **`Self`**（运行期不知 `Self` 大小）。

→ 用 **`where Self: Sized`** 把「返回 `Self`」等方法**排除在 trait object 调用之外**，trait 仍可 `dyn`。

---
