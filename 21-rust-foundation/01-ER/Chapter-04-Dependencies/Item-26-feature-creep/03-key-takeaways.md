# Item 26 · 重点结论

← [Item 26 目录](./README.md)

### Features 必须是加法的（Additive）

- ❌ 互斥组合（A 架构 vs B 架构）→ 别做成 feature；用 **`cfg(target_arch = "...")`** 等 target cfg。
- ❌ 否定语义（`no_std` feature 删代码）→ 用 **`std` feature 加代码**（Clippy 会 warn 否定名）。

### 不要给 pub 字段 / trait 方法加 feature 门控

- 用户无法预知 unification 是否静默开启 feature → 结构体字面量 / impl 突然缺字段或方法。

### 命名空间

- Crate **名**与 **feature 名**同空间 → 避免与常见依赖名冲突。

### 组合爆炸

- \(N\) 个独立 feature → 最多 \(2^N\) 种组合。
- **少开 feature**；CI 用 **`cargo-hack --feature-powerset`**（§6）穷举测编译。

---
