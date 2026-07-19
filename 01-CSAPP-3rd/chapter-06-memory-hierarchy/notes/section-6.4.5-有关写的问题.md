## 6.4.5 有关写的问题

> **Ch6 §6.4.5** · [章导读](../README.md) · 上节 [§6.4.4 ←](./section-6.4.4-全相联.md) · 下节 [§6.4.6 →](./section-6.4.6-真实Cache层次解剖.md)

---

| 策略 | 行为 |
|------|------|
| **写直达 (write-through)** | 写同时更新 cache 与下层 — 简单，总线忙 |
| **写回 (write-back)** | 只写 cache，**dirty** 位；替换时才写回 — 常用 |
| **写分配 (write-allocate)** | miss 时先 load line 再写 — 利用局部性 |
| **非写分配** | miss 直接写下层 — 少用于 L1 |

- **store 引发 miss** — 可能触发 load line（与 Ch5 load 性能联动）

---

### 口述巩固 · 自测

1. （待口述补）本节核心一句话？

---

← [§6.4.4 ←](./section-6.4.4-全相联.md) · [本章导读](../README.md) · [§6.4.6 →](./section-6.4.6-真实Cache层次解剖.md)
