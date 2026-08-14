# Item 35 · 逻辑脉络

← [Item 35 目录](./README.md)

```text
C 源码 ↔ C 头文件（C 编译器检查）
         ↓
bindgen(同一 .h) → Rust 声明
         ↓
人工手写 Rust 声明 → 易 drift，无检查
         ↓
CI：重新 bindgen → git diff 与签入版不一致则 fail（Item 32）
         ↓
xyzzy-sys（unsafe 生成码）+ xyzzy（safe 封装）→ Item 16
```

---
