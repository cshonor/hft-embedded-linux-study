# 1 · 安全受检的未初始化内存 (Checked)

← [本章目录](./README.md) · [00-overview.md](./00-overview.md) · 下一节：[02-drop-flags.md](./02-drop-flags.md)

---

## 静态分析

栈变量在显式赋值前未初始化；Rust 用**基本分支分析**在编译期阻止赋值前读取。

→ 源码：[src/checked.rs](./src/checked.rs)（条件初始化、`if/else` 全路径赋值）

## 逻辑未初始化

非 `Copy` 值被**移出**后，原变量在逻辑上重新变为未初始化，不可再使用。

→ 源码：[src/checked.rs](./src/checked.rs)（move 后不可再用 — 注释 + 编译器 enforced）
