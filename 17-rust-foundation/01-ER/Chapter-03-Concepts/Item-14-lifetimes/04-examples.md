# Item 14 · 案例与代码

← [Item 14 目录](./README.md)

### `Box::leak` → `'static`

```rust
let boxed = Box::new(Item { contents: 12 });
let r: &'static Item = Box::leak(boxed);
// 堆不再被 Drop；引用可视为 'static（慎用：泄漏）
```

### 返回带引用的 struct：用 `'_`

```rust
// 不推荐：隐藏与 items 的绑定
// pub fn find_one(items: &[Item]) -> ReferenceHolder

pub fn find_one(items: &[Item]) -> ReferenceHolder<'_> {
    // ...
}
```

### 临时值 + 借用

```rust
// fn_returning_ref(&mut Item { contents: 42 }) // 临时在语句末 drop
let mut tmp = Item { contents: 42 };
let r = fn_returning_ref(&mut tmp); // ✅ 先绑定具名变量
```

---
