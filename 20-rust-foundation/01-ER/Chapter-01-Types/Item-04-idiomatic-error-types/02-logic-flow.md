# Item 4 · 逻辑脉络

← [Item 4 目录](./README.md)

```text
String 当错误（不行：无法 impl Error）
  → Newtype 包装（MyError(String)）
  → Enum 聚合子错误 + 自定义 source()
  → 库：具体 Enum + thiserror
  → 应用：Box<dyn Error> + anyhow
```

### Newtype 突破孤儿规则

- `String` 不能直接挂 `Error`。
- `struct MyError(String);` 是**新类型**，可在本 crate `impl Display + Error`。

### 枚举保留诊断细节

- 不同失败原因（I/O vs UTF-8 vs 业务）→ **`enum MyError { Io(...), Utf8(...), ... }`**
- 在 `source()` 里把内部 `io::Error` 等**链出去**。

### 特征对象 vs 一致性（Coherence）

- 想统一异构错误：`Box<dyn Error>` 包装。
- 若再手写 `impl<E: Error> From<E> for MyWrapper` 等，易与标准库 **blanket impl**（如 `From<T> for T`）**冲突** → 应用层优先用 **`anyhow`**。

---
