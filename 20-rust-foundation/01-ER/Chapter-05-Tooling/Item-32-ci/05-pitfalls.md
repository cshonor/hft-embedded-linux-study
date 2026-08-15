# Item 32 · 易错细节

← [Item 32 目录](./README.md)

| 陷阱 | 说明 |
|------|------|
| **CI 里 bench 当真相** | 共享容器噪声大 → 性能基线需**专用硬件**（Item 30 / 31） |
| **fuzz 进每次 PR** | 太慢；单独 scheduled workflow |
| **只测 default features** | 漏 \(2^N\) 组合断层（Item 26） |
| **库项目只测带 lock** | 下游无 lock → 加 **lockfile-free** job |
| **toolchain 浮动** | 今天绿明天红，难归因 |

---
