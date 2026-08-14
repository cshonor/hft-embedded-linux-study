# Item 32 · 重点结论

← [Item 32 目录](./README.md)

### 别浪费人类时间

- CI 步骤应能在**本地一条命令**复现 → 作者 PR 前自修，审查者看设计而非 fmt 吵架。

### 消灭 flaky tests

- 间歇失败 = **立即根因修复**或删/隔离；不能「习惯红着」。

### 绝对全绿

- 不接受「那个测试一直挂」「这个 clippy 误报不管」→ 破窗 = 新退化看不见。

### 开源 CI 安全

- 限制 **fork PR 自动跑**（或只读 token）；新贡献者 **manual approval**。
- 第三方 Action **钉 commit SHA**；写权限 step 最小化；防挖矿 / 窃 token / 供应链。

---
