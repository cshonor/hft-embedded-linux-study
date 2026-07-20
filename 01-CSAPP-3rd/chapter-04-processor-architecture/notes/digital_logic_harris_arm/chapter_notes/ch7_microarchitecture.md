# Ch7 · 微结构（↔ CSAPP Ch4 主战场）

> **Core Concept:** Single-cycle · multicycle · **pipeline + hazards**; optional advanced µarch
> **Link Target:** CSAPP §4.3 SEQ · §4.4 · §4.5 PIPE · HFT

## 目录锚点

- 7.1–7.2 引言、性能分析
- **7.3 单周期处理器**（数据通路/控制）≈ SEQ
- 7.4 多周期 — 中读
- **7.5 流水线处理器：数据通路、控制、冲突、性能** ≈ PIPE
- `*7.6` HDL 表示 — 跳过
- `*7.7` 高级：深流水、分支预测、超标量、乱序、多线程… — HFT **建议浅读**
- `*7.8` ARM 微结构演变 — 可选
- 7.9 总结

## Status

- [ ] 7.3 ↔ SEQ
- [ ] **7.5 冲突** ↔ §4.5
- [ ] `../cross_ref/csapp_ch4_link.md` 写满
- [ ] `../cross_ref/hft_x86_timing.md`
