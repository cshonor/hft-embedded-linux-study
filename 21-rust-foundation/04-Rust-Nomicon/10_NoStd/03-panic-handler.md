# 3 · 自定义恐慌处理 (`#[panic_handler]`)

← [本章目录](./README.md) · 上一节：[02-no-main.md](./02-no-main.md) · 下一节：---

无 std 时默认 `panic!` **失效**，须定义：

```rust
#[panic_handler]
fn panic(_info: &PanicInfo) -> ! { ... }
```

- 签名严格：`fn(&PanicInfo) -> !`
- 整个依赖图**只能有一个**

**多环境策略**（Cargo profile / features）：

| 环境 | 典型 crate |
|------|------------|
| dev 调试 | `panic-semihosting`（输出到主机） |
| release | `panic-halt`（死循环挂起） |

→ 源码：[src/lib.rs](./src/lib.rs)（`panic_halt` 示例）
