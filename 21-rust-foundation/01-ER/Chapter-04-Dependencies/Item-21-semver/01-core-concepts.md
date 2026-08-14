# Item 21 · 核心知识点

← [Item 21 目录](./README.md)

### Semver（Semantic Versioning）

Cargo 用 Semver 解析依赖。格式 **`MAJOR.MINOR.PATCH`**：

| 段 | 何时递增 |
|----|----------|
| **MAJOR** | **不兼容** API 变更 |
| **MINOR** | **向后兼容**地加功能 |
| **PATCH** | **向后兼容** Bug 修复 |

### 发布不可变

- 某版本一旦发布 → **内容不可改**；任何变动必须是**新版本**（Book 14.2；yank 只阻止新解析，不删源码）。

### `0.y.z` 特殊规则（left-shifting）

- `0.x` = 初始开发，API 随时可能 break。
- Cargo：**最左侧非零位**变化即视为不兼容：
  - `0.2.3` → `0.3.0` ✓ break
  - `0.0.4` → `0.0.5` ✓ break

---
