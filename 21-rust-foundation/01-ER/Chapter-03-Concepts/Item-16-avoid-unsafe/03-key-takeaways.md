# Item 16 · 重点结论

← [Item 16 目录](./README.md)

### 默认策略：极其克制

遇到「好像只能 unsafe」的问题 → **先查 std / crates.io**，多半已有安全封装。

### 必须写 unsafe 时的防御准则（*Hic sunt dracones*）

| 准则 | 做法 |
|------|------|
| **Safety Comments** | 写清 `unsafe` 依赖的先决条件与不变量（Clippy 会提醒） |
| **最小爆炸半径** | 缩小 `unsafe { }` 范围；开 `unsafe_op_in_unsafe_fn`，`unsafe fn` 内也显式写块 |
| **加倍测试** | 边界、异常路径、回归 |
| **Miri** | 解释 MIR，抓静态分析抓不到的 **UB** |
| **安全封装隔离** | unsafe 在内层；对外只暴露 safe wrapper API |

---
