# 7. Summary（小结）

> [← 章索引](./README.md)

```text
no_std → alloc? → runtime(panic/init/OOM) → volatile → typestate HAL → cross-build
```

## 三句话

1. **`core` / `alloc` / `std`** 分层清晰再选依赖。  
2. **panic / 启动 / OOM** 须自己契约。  
3. **类型状态 + volatile** 把硬件错误前移到编译期 / 明确语义。

## 下一章

→ [第 13 章 Ecosystem](../Chapter-13-Rust-Ecosystem/13-生态系统-Ecosystem-深度解析.md)

## 索引

[01](./01-opting-out-no-std.md)–[08](./08-cross-compilation.md)
