# Item 16 · 逻辑脉络

← [Item 16 目录](./README.md)

```text
Rust 卖点：零开销 + 编译期内存安全
         ↓
底层需求：OS API、硬件、极限性能 → 需要「逃生舱」unsafe
         ↓
Item 16 立场：避免 **编写 (writing)**，优先 **复用 (reuse)**
         ↓
std / crates.io 里已有专家封装 → 用安全 API，别重写 unsafe
```

---
