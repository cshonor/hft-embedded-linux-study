# Item 35 · 核心知识点

← [Item 35 目录](./README.md)

### 手工 FFI 的风险（Item 34）

- 手写 `extern "C"` / `#[repr(C)]` → **链接器不验证** C 与 Rust 声明一致。
- 略有不匹配 → **静默** → 运行时崩溃。

### `bindgen`

- 解析 **C 头文件** → 自动生成 Rust `extern` / struct / enum 等声明。
- 与 C 编译器共用**同一份 `.h`** → 接口同步链。

### `cxx`

- Rust ↔ **C++** 互操作；从共享 **schema** 同时生成两侧绑定。
- 比 bindgen 扫 C++ 更**类型安全**、更完整。

---
