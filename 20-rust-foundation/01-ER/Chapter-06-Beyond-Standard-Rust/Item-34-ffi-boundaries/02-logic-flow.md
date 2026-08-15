# Item 34 · 逻辑脉络

← [Item 34 目录](./README.md)

```text
Rust 侧：借用 + 生命周期（Item 14/15）
         ↓ 过 FFI
C 侧：裸指针，无借期、可 UAF、可 data race
         ↓
重建安全：unsafe 在内，safe wrapper 对外（Item 16）
         ↓
所有权跨界：同侧 alloc/free；Box ↔ 裸指针（Item 11 Drop）
         ↓
panic 不得越过 FFI（Item 18 → catch_unwind / abort）
```

---
