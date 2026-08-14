# Item 30 · 逻辑脉络

← [Item 30 目录](./README.md)

```text
单元测试 ── 内部细节
集成 / doc test ── 边界与契约
examples ── 用户视角整体用法
bench ── 性能（Item 20：先测再优化）
fuzz ── 不可信输入（解析器、网络协议…）
         ↓
类型系统已保证的（Item 1）→ 不必重复测
依赖行为漂移（Item 21）→ 测试作早期预警
         ↓
CI：matrix × features（Item 26/32）；fuzz 单独定期跑
```

---
