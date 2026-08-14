# Item 12 · 易错细节

← [Item 12 目录](./README.md)

### Upcasting 误区

- 不要以为 **`&dyn Shape` 能随意当 `&dyn Draw`** 传递（即使 `Shape: Draw`）。
- 旧版 Rust：`Shape` vtable 是融合体，**不能**在运行期「剥离」成纯 `Draw` vtable。
- **Rust 1.76+**：`trait_upcasting` 逐步支持部分向上转型 → 见 §6。

### 性能盲区

`dyn` 方法调用路径大致：

```text
胖指针 → 读 vtable → 取函数指针 → 间接跳转（难内联）
```

热点路径优先泛型；用 bench 验证。

---
