# Item 32 · 核心知识点

← [Item 32 目录](./README.md)

### 持续集成（CI）

- 代码变更 / PR 时**自动**跑检查与测试。

### 确定性构建

- 步骤须**干净、快、确定、低误报**。
- 用 **`rust-toolchain.toml`** 钉死 toolchain：

```toml
[toolchain]
channel = "1.70.0"
# 或 channel = "nightly-2023-09-19"
```

- 避免 CI 用浮动 `stable` 而**无声**随新版本变红/变绿。

### 分级节奏（Cadence）

| 层级 | 时机 | 典型内容 |
|------|------|----------|
| 本地 / IDE | 保存 / pre-commit | fmt、rust-analyzer、clippy |
| **PR** | 每次审查 | check、test、clippy、fmt --check |
| **merge main** | 合并后 | 集成测试、多 target |
| **定期** | 日/周 | feature powerset、MSRV、无 lock 构建 |
| **后台** | 持续 | fuzz corpus 扩展（Item 30） |

---
