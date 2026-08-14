# 4. Summary（小结）

> [← 章索引](./README.md)

```text
extern/ABI → 类型/分配/回调/安全 → bindgen 工程化
```

## 三句话

1. FFI = **ABI + repr + 所有权** 契约。  
2. **panic 不穿越**；用 **catch_unwind + 错误码**。  
3. **`*-sys` + 安全层** + 生命周期 / 新型别擦除 `void*`。

## 下一章

→ [第 12 章 no_std](../Chapter-12-Rust-Without-Standard-Library/README.md)

## 索引

[01](./01-symbols.md)–[07](./07-bindgen-build-scripts.md)
