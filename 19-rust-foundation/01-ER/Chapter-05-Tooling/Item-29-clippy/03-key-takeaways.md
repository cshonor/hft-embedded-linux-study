# Item 29 · 重点结论

← [Item 29 目录](./README.md)

### 目标：Clippy-warning free

- **修** 或 **显式 `allow`（并注释为何）** —— 二选一，别留着不管。
- 基线非零 → 新警告被噪音淹没（破窗效应）。

### 别为「误报」死磕

- 多数情况：轻微重构顺应 Clippy **比团队争论成本低**。

### CI 必跑

```bash
cargo clippy -- -Dwarnings
```

- 警告升级错误 → 不合规代码进不了 main（Item 32）。

### Pedantic 组

- 默认关的 **`clippy::pedantic`** 等极严规则 —— 可不全开，但**读 lint 说明**能加深 Rust 细节理解（如对 `as` 的 `cast_*` 系列）。

---
