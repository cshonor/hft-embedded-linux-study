# Item 22 · 逻辑脉络

← [Item 22 目录](./README.md)

```text
默认私有 → 只暴露必要 API
         ↓
pub 扩大可见性 = 消耗未来重构自由（Semver 单向门）
  私有 → 公开：Minor（兼容）
  公开 → 私有：Major（break）
         ↓
字段/内部类型公开 → 换实现 = Major
隐藏细节 → 内部可优化而不 break 用户
```

与 Item 21 联动：**API 面越小，Major 升级压力越小**。

---
