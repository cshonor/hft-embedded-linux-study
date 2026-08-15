# 第 10 章 · 语义分析（2）静态类型检查

> **《自制编译器》** · [03 Build Your Own Compiler](../../README.md) · [本书目录](../../本书目录.md) · 第2部分 · 抽象语法树和中间代码

## 状态

- [x] 已读（笔记整理）

---

## 一句话

**语义 Pass 收官（IR 前）** — ① **类型定义检查**（void 成员 · 重复成员 · **结构体循环定义**=类型图 **DFS**）② **表达式有效性**（左值/赋值/`*`/`&` · **`DereferenceChecker`** + **try-catch** 防连锁错 · **数组/函数→指针** decay）③ **`TypeChecker`** **自下而上** 推类型 · 运算限制 · 隐式 **`CastNode`**（**整型提升** · **寻常算术转换**）。

---

## 专项笔记（一小节一文件）

| 小节 | 主题 | 阅读 |
|------|------|------|
| — | 本章定位 | [00-overview.md](./00-overview.md) |
| §1 | 类型定义的检查 | [01-type-definition-check.md](./01-type-definition-check.md) |
| §2 | 表达式的有效性检查 | [02-expression-validity.md](./02-expression-validity.md) |
| §3 | 静态类型检查 | [03-static-type-check.md](./03-static-type-check.md) |
| — | 速记 · 自测 |

---

## 与仓库其他部分

| 本书 ch10 | 对照 |
|----------|------|
| ch9 消解 | [chapter09_name-resolution](../chapter09_name-resolution/README.md) · 阶段 1～2 |
| ch11 IR | 语义正确 AST → 中间代码 |
| EaC ch4 | [上下文/类型系统](../../../02_Compiler-Principles/chapter04_context/) |
| Rust | 类型检查 + **无隐式整型提升** — 对照 C 规则 |

---

## 逻辑脉络

类型定义合法 → 表达式可求值 → 类型一致与转换。

---

## 速记

## 本章速记

```text
§1  void成员/重复成员/循环struct · 类型图DFS判环
§2  lvalue/赋值/*/& · DereferenceChecker+try-catch · array/func→ptr
§3  运算限制 · CastNode · 整型提升 · usual conversion · TypeChecker↑
```

---

## 三句背诵

1. **类型定义用图 DFS 抓循环 struct。**
2. **有效性先拦 1=2；catch 防报错泛滥。**
3. **TypeChecker 自底向上，不够就插 CastNode。**

---

## ch9～10 语义五步

| 步 | 章 |
|:--:|-----|
| 1～2 | ch9 |
| 3～5 | **ch10** |

---

## 自测

- [ ] 循环 struct 检测三状态
- [ ] `&1` 为何非法
- [ ] 整型提升与寻常算术转换区别
- [ ] CastNode 在 AST 中的作用

---

## 阅读进度

- [x] ch10 语义分析（2）静态类型检查
- [ ] ch11 中间代码的转换

