# Item 34 · 核心知识点

← [Item 34 目录](./README.md)

### FFI（Foreign Function Interface）

- Rust 与其他语言互操作；常以 **C ABI** 为「最小公约数」（无 GC、无异常、无模板）。

### 不安全边界

- 跨 FFI = 离开 Rust 内存安全保证 → **所有 FFI 调用 `unsafe`**（Item 16 原则在此**必须打破**，但须封装）。

### 名称修饰

| 机制 | 作用 |
|------|------|
| **`extern "C"`** | C ABI；隐式 **`#[no_mangle]`** → 符号名不被 Rust 编码 |
| C++ / Rust 默认 | name mangling → 与 C 全局命名空间不兼容 |

### C 兼容布局

```rust
#[repr(C)]
pub struct FfiPoint {
    pub x: u32,
    pub y: u32,
}
```

- 禁止 Rust 默认字段重排 → 与 C struct **布局、对齐一致**。

---
