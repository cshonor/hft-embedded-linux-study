# 第 10 章 · 语义分析（2）静态类型检查 · 本章定位

← [本章目录](./README.md) · 上一章：[ch9 语义分析（1）](../chapter09_name-resolution/README.md) · 下一章：[ch11 中间代码的转换](../chapter11_ir/)（待建） · 下一节：[01-type-definition-check.md](./01-type-definition-check.md)

---

```text
ch9   变量/类型名消解
  ↓
ch10  类型定义 · 表达式合法 · 静态类型检查  ← 本章（语义 3～5 步）
  ↓
ch11  IR 生成
```

| [ch9 五阶段](../chapter09_name-resolution/01-semantic-overview-visitor.md) | 本章 |
|-----------------------------------------------------------------------------|------|
| 步骤 3～5 | **IR 前** 必须语义正确 |

**产出**：带 **`Type`**、插入 **`CastNode`** 的 AST — `TypeChecker` 遍历完成。
