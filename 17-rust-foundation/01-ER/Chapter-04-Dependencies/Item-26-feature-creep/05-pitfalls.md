# Item 26 · 易错细节

← [Item 26 目录](./README.md)

| 陷阱 | 说明 |
|------|------|
| **`default` 不是魔法** | 只是普通 feature 名；任一依赖未 `default-features = false` → unification **仍开 default**（Item 25） |
| **`no_std` 当 feature 名** | 违反加法；A 开 B 不开 → 灾难；用 **`std`** 正向开启 |
| **optional dep = 隐式 feature** | 忘记在 `[features]` 里文档化何时该开 |
| **\(2^N\) 未测** | 某组合从未 CI 构建 → 用户 первый 踩雷 |

---
