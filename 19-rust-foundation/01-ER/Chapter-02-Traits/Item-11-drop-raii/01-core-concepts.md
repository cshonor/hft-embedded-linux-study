# Item 11 · 核心知识点

← [Item 11 目录](./README.md)

### RAII（Resource Acquisition Is Initialization）

- **资源获取即初始化**：对象**创建**时拿到资源，**销毁**时释放资源。
- **不变量**：只有该类型的**实例存在**时，才合法访问底层资源；实例离开作用域 → 资源必释放。

### `Drop` trait

```rust
trait Drop {
    fn drop(&mut self);
}
```

- 内存回收**前**由编译器**自动**调用（离开 `{}`、变量被 move 走且旧值 drop 等）。
- **默认**：只递归 drop 字段、释放内存。
- **手写 `impl Drop`**：**替换**默认路径 —— 先跑你的 `drop`（外部资源），再自动 drop 字段；**不是叠加** → [15.3 §二](../../../00-Book/15-smart-pointers/15.3-使用Drop运行清理代码.md#二默认-drop-vs-手写-drop替换不是叠加)
- 与 C++ 析构函数同类思想；Rust 用**所有权 + 作用域**强制配对。

---
