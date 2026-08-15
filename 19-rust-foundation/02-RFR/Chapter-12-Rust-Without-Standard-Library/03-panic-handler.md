# 3.1 The Panic Handler（Panic 处理器）

> 所属：**The Rust Runtime** · [← 章索引](./README.md)

`no_std` 下**必须**提供 **`#[panic_handler]`**：

```rust
#[panic_handler]
fn panic(_info: &core::panic::PanicInfo<'_>) -> ! {
    loop {} // 或日志、复位 — 依平台而定
}
```

- **不得**假设 `std` 默认 unwind / 打印
- 与 **`panic = "abort"`** profile 策略一致

→ [第 9 章 · Panic](../Chapter-09-Unsafe-Code/07-panics.md)
