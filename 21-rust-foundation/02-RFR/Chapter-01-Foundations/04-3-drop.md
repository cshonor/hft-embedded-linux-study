# 04.3 · Drop 是什么 · `Box` · 自定义 Drop

← [04 所有权索引](./04-ownership.md) · 上一节：[04-2-move-copy-clone.md](./04-2-move-copy-clone.md) · 下一节：[04-4-drop-order.md](./04-4-drop-order.md)

---

## Drop 基础

- 类型可实现 **`Drop` trait**：离开作用域时编译器插入 **`drop()`** 调用。
- **`String` / `Vec` / `Box`**：drop 时释放堆（见 [03.1](./03-1-rust-memory-model.md)）。
- **引用 `&T` / `&mut T`**：**不拥有**值，drop 引用**不会**释放被指向的数据；只有**所有者** drop 时才释放。

```rust
let x = String::from("hi");
let k = &x;     // k 是借用，不是所有者
// 作用域结束：只 drop x（释放堆）；k 作为引用无堆责任
```

---

## `Box<T>`、嵌套类型的 drop

```rust
let b = Box::new(String::from("hi"));
// drop 时：先 drop Box 管理的堆上 String，再清理 Box 自身（栈上句柄）
```

- **`Box<String>`**：栈上 `Box` 句柄 + 堆上 `String`；drop **`Box` 时**按 `Box` 的规则释放堆上 `T`。
- **struct 多字段**：外层 struct 按字段正序 drop；每个字段再按自己的类型规则 drop（如 `String` 释堆）。

---

## 自定义 `Drop`：亲眼看到逆序

局部变量按**逆声明** drop（见 [04-4](./04-4-drop-order.md)）：

```rust
struct Custom {
    name: String,
}

impl Drop for Custom {
    fn drop(&mut self) {
        println!("Dropping Custom: {}", self.name);
    }
}

fn main() {
    let a = Custom { name: "A".into() };
    let b = Custom { name: "B".into() };
}
// 输出：
// Dropping Custom: B
// Dropping Custom: A
```
