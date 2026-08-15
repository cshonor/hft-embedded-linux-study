# 第 2 章 · 扫描 · §6 关键字识别与工程权衡

← [本章目录](./README.md) · 上一节：[05-scanner-implementation.md](./05-scanner-implementation.md) · 下一节：---

## 问题

若把语言**所有关键字**（`if`、`while`、`fn`…）都**硬编码进 DFA**：

- DFA **状态数**膨胀
- **转换表**急剧变大
- 每增一个关键字可能重构整张表

---

## 常用策略（现代扫描器）

```text
1. DFA 先把 if / while / myVar 统一识别为「标识符」
2. 查预先建立的散列表（Hash Table）
3. 命中 → 改写 Token 类型为对应关键字
4. 未命中 → 保持普通标识符
```

| 步骤 | 效果 |
|------|------|
| DFA 只认「标识符形状」 | **大幅简化**自动机 |
| 散列表区分关键字 | O(1) 均摊查表，关键字数量可扩展 |

---

## 与 CI Lox 扫描器

Lox 扫描器通常：**先读字母开头的 lexeme**，再 **switch / map 查 reserved word** — 与橡书策略一致。

→ [CI jlox ch4 Scanning](../../../01_Crafting-Interpreters/part02_jlox/chapter04_scanning/README.md)

---

## 本章收束

| 层 | 内容 |
|----|------|
| **理论** | RE → NFA → DFA → 最小化 |
| **实现** | 表驱动 vs 直接编码 |
| **工程** | 关键字散列、DFA 瘦身 |

> 严谨自动机理论 + 散列表等工程技巧 → **既正确又高速**的扫描器。

---

## 延伸

- **最长匹配 / 前瞻**：`<<` vs `<`、`/*` 注释 — 实际 lexer 在 DFA 上还有**匹配策略**细节（CI 笔记中有实操）。
- **错误恢复**：非法字符时报告位置并同步 — 属扫描器 UX（[ch1 §5 反馈](../chapter01_overview/05-desired-properties.md)）。
