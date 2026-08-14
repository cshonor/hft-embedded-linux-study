# Item 30 · 重点结论

← [Item 30 目录](./README.md)

### 按场景选工具

| 场景 | 工具 |
|------|------|
| 解析外部/不可信输入 | **fuzz**（必须） |
| 性能是核心 SLA | **bench** + 跟踪结果 |
| 修 bug | **先写复现测试**（TDD） |
| 多 feature / 多 target | CI **matrix + cargo-hack**（Item 26） |

### 测试代码 vs 业务代码（Item 18）

- 测试里 **`unwrap` / `expect` / `#[should_panic]`** ✅ — panic = 失败信号。
- **`examples/` 里大量 `unwrap`** ❌ — 用户会复制粘贴；用 `fn main() -> Result<…>` + `?`。

### Bench 注意

- 编译器可能 **const-fold** → `0 ns/iter` → 用 **`black_box`**（§4）。
- 稳定版 bench 支持有限；精确统计用 **`criterion`**（Item 31）。

### Fuzz 与 CI

- Fuzz **开放式、耗时长** → 不适合每次 PR；**定期 / 发布前**跑，维护 **corpus**（§6）。

---
