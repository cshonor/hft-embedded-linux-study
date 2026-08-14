# 1.3 Rust Tooling（Rust 工具链）

> 所属：**What's Out There?** · [← 章索引](./README.md)

## Rustup

```bash
cargo +nightly test    # 单次 nightly（Miri、-Z 标志）
rustup doc             # 离线 std 文档
rustup component add miri
```

## 编译器洞察

- **`rustc -Zprint-type-sizes`**（通常 nightly）— 类型大小、对齐、布局
- 与 [第 2 章 Layout](../Chapter-02-Types/02-layout.md)、[06_Compilers-and-LLVM-Learning/04_Learn-LLVM-17](../../06_Compilers-and-LLVM-Learning/04_Learn-LLVM-17/README.md) 配合

## 编辑器

本仓库 → [`docs/rust-analyzer-VSCode配置.md`](../../docs/rust-analyzer-VSCode配置.md)
