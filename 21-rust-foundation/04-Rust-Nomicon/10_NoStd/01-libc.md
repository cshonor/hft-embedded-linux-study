# 1 · 引入 libc 依赖

← [本章目录](./README.md) · [00-overview.md](./00-overview.md) · 下一节：[02-no-main.md](./02-no-main.md)

---

`#[no_std]` 可执行文件常需 **`libc`**，且必须：

```toml
libc = { version = "...", default-features = false }
```

**`default-features = true` 会隐式拉回 `std`**，破坏 no_std 构建。

→ 见 [Cargo.toml](./Cargo.toml) 注释
