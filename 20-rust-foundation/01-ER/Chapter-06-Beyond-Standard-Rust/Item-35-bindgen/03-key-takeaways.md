# Item 35 · 重点结论

← [Item 35 目录](./README.md)

### 默认：用 bindgen，别手写

- 除**极简**一两个符号外，一律机器生成。

### `-sys` + 安全层双层架构

| Crate | 职责 |
|-------|------|
| **`xyzzy-sys`** | `bindgen` 输出；**全 unsafe** |
| **`xyzzy`** | 封装为 safe Rust API；业务只依赖此层 |

→ 应用代码继续遵守 Item 16。

### C++ → `cxx`，不是 bindgen

- bindgen 对 C++ **子集有限**；紧密 C++/Rust 互操作用 **`cxx`**。

---
