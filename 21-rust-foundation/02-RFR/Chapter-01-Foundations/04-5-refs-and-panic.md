# 04.5 · 引用与 move · panic / unwind

← [04 所有权索引](./04-ownership.md) · 上一节：[04-4-drop-order.md](./04-4-drop-order.md) · 下一节：[04-6-pitfalls.md](./04-6-pitfalls.md)

---

## 引用与 move

| | 是否拥有 | 赋值时 | 作用域结束 |
|---|----------|--------|------------|
| **`String` / `Box<T>`** | 拥有 | 默认 **move** | **Drop** 释放资源 |
| **`&T` / `&mut T`** | 不拥有 | **Copy**（复制指针+生命周期） | 只结束借用，**不** drop 目标 |

→ 借用细节 [05 共享引用](./05-shared-references.md) · [06 可变引用](./06-mutable-references.md)

---

## panic、unwind 与 Drop

| 情况 | Drop 行为 |
|------|-----------|
| **正常 return** | 按上述顺序 drop 栈上变量 |
| **panic + unwind**（默认） | 栈展开，**仍按正常 Drop 顺序**析构已构造的对象 → 一般不泄漏 |
| **`catch_unwind`** | 捕获 panic；**被展开帧里的 Drop 已执行** |
| **abort 模式**（`panic=abort`） | **不 unwind**，栈上 Drop **可能不跑** → 依赖 OS 回收 |

```rust
use std::panic;

let r = panic::catch_unwind(|| {
    let s = String::from("leak?");
    panic!("boom");
});
// unwind 路径上 s 仍会被 drop
```

**实践**：需要 panic 时也释放锁/文件 → 用 `Drop` / RAII；关键资源勿假设 abort 下会 drop。
