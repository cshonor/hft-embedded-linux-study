# Item 32 · 逻辑脉络

← [Item 32 目录](./README.md)

```text
ER 全书建议（Clippy、deny、features、doc…）
         ↓
仅文档 / 口头 → 很快腐化
         ↓
CI 自动化 → 真正防线
         ↓
流程 Bug（忘跑 codegen）→ 先加 CI 步骤再修（同 Item 30 TDD）
         ↓
全绿铁律 + 本地可复现 → 人不替机器背锅
```

---
