# 第 1 章 · 开始制作编译器 · 本章定位

← [本章目录](./README.md) · [03 总览](../00-overview.md) · 下一章：[ch2 C♭ 和 cbc](../chapter02_cflat-cbc/README.md) · 下一节：[01-book-overview.md](./01-book-overview.md)

---

```text
ch1  地图：做什么 · 分几步 · 先跑起来
  ↓
ch2  C♭ 语言 + cbc 源码结构
  ↓
第1部分 ch3～6  代码分析（动手写 parser）
```

| 对比 | 本书 ch1 | [EaC ch1](../../../02_Compiler-Principles/chapter01_overview/README.md) | [CI ch1](../../../01_Crafting-Interpreters/part01_welcome/chapter01_introduction/README.md) |
|------|----------|---------------------------------------------------------------------------|---------------------------------------------------------------------------------------------|
| 侧重 | **真 ELF 编译器**路线 + 一次 demo | 工程原则与三阶段 | 为何学 · 书怎么读 |

**读完应能回答**：C♭ 是什么、cbc 产出什么、从 `.cb` 到 `./hello` 经过哪些阶段。
