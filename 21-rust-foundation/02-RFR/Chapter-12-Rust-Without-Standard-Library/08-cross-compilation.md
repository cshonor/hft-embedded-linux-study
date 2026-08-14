# 6. Cross-Compilation（交叉编译）

> [← 章索引](./README.md)

## Target triple

如 **`thumbv7m-none-eabi`** — 指令集、ABI、是否带 OS。

## 工具链

```bash
rustup target add thumbv7m-none-eabi
cargo build --target thumbv7m-none-eabi
```

## 自定义 target

冷门平台 → **`target-spec` JSON** 传给 `rustc`（字段以官方文档为准）。

## 链接

链接器 / 运行库由平台生态（**`cortex-m-rt`** 等）补齐。
