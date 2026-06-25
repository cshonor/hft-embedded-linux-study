# 03 · 30 天自制 CPU

> **父模块：** [09-system-low-level-hands-on](../README.md)  
> **参考：** 矢泽久雄《30 天自制 CPU》· 本仓库为 HFT 学习链裁剪笔记 + 实验记录

## 定位

从 **门电路 / 逻辑表** 搭出能跑汇编的最小 CPU — 把 CSAPP / Hennessy 里的「周期、流水线、取指」从 **名词** 变成 **你造过的东西**。

## 建议阶段（待按原书章节补 OUTLINE）

| 阶段 | 主题 | HFT 关联 |
|------|------|----------|
| Day 1–10 | 逻辑门、加法器、寄存器 | 位级运算与状态机 |
| Day 11–20 | 数据通路、控制单元、指令集 | **单周期延迟** 从哪来 |
| Day 21–30 | 汇编器、程序加载、运行测试 | 对照 `perf` 热点与指令 mix |

## 产出

- [ ] `OUTLINE.md` — 按原书 Day 裁剪 🔴/🟡/⚪
- [ ] `code/` — 电路/汇编实验（按所用工具链）
- [ ] 与 [04-Hennessy](../../04-Computer-Architecture-6th/) Ch2–3 对照表

## 交叉阅读

- [01-CSAPP Ch4/5](../../01-CSAPP-3rd/chapter-04-processor-architecture/) · [04-Hennessy](../../04-Computer-Architecture-6th/)
- 上一步：[01-30days-os](../01-30days-os/)
- 进 HFT：[15-HFT-Low-Latency-Practice](../../15-HFT-Low-Latency-Practice/)
