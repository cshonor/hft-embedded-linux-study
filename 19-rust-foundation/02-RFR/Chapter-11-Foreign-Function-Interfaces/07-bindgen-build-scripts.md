# 3. bindgen and Build Scripts

> [← 章索引](./README.md)

## bindgen

从头文件 **`.h`** 生成 Rust FFI 绑定 — 减少手写 `extern` 错误。

## 分工

| Crate | 职责 |
|-------|------|
| **`foo-sys`** | 裸绑定 + 链接 + 版本钉扎 |
| **`foo`** | 安全封装 API |

## build.rs

- 告诉 Cargo **链接**哪些 native 库
- 重跑条件（`rerun-if-changed`）
- 调用 bindgen 生成 `bindings.rs`

## 其它工具

- **`cbindgen`** — Rust 导出 C 头文件
- **`cxx`** — C++ 类型安全桥梁（名称修饰 / 异常复杂时）

Demo → [Item 35 bindgen](../../01-ER/Chapter-06-Beyond-Standard-Rust/Item-35-bindgen/README.md)
