# Item 34 · 易错细节

← [Item 34 目录](./README.md)

| 陷阱 | 说明 |
|------|------|
| **手写 `extern` 签名错** | `extern "C"` 无类型检查链接 → **运行时才崩**；用 **bindgen**（Item 35） |
| **签名略有不一致** | 链接器仍通过 → 栈/layout 错乱 |
| **混用 allocator** | 跨语言 free = 经典 crash |
| **FFI crate 多版本** | C 符号 ODR 冲突（Item 25） |
| **panic 泄漏** | UB |

---
