# Item 15 · 逻辑脉络

← [Item 15 目录](./README.md)

```text
C/C++ 裸指针：无借期、无读写互斥 → UAF / 数据竞争
Rust 引用 & / &mut + 借用检查器 → 编译期拦截

出借期间 owner 被压制：
  存在 &T   → owner 可读，不可改 / move / drop
  存在 &mut → owner 连读都不行，直到 &mut 结束
```

---
