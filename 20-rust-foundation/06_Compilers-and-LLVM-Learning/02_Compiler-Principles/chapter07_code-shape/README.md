# 第 7 章 · 代码形态（Code Shape）

> **Engineering a Compiler 3e** · [02 编译器工程](../../README.md) · [本书目录](../../本书目录.md) · Part II 基础结构

## 状态

- [x] 已读（笔记整理）

---

## 一句话

**代码形态 = 为表达某一源语言结构而生成的具体 IR/指令序列**。同一高级结构可有多种 lowering；选型影响**优化空间**与**目标代码质量**。本章是 **前端 → 后端过渡的工程指南**：存哪儿、算符怎么译、数组/结构体怎么寻址、控制流/ switch 怎么布、调用/OOP 怎么落码 — 衔接 [ch5 IR](../chapter05_ir/README.md) 与 [ch6 过程](../chapter06_procedures/README.md)。

---

## 专项笔记（一小节一文件）

| 小节 | 主题 | 阅读 |
|------|------|------|
| — | 本章定位 | [00-overview.md](./00-overview.md) |
| §1 | 指定存储位置 · 歧义值 | [01-storage-locations.md](./01-storage-locations.md) |
| §2 | 操作符的底层转换 | [02-translating-operators.md](./02-translating-operators.md) |
| §3 | 复杂数据结构 | [03-data-structures.md](./03-data-structures.md) |
| §4 | 控制流结构 | [04-control-flow.md](./04-control-flow.md) |
| §5 | 过程调用与 OOP 扩展 | [05-calls-and-oop.md](./05-calls-and-oop.md) |
| — | 速记 · 自测 |

---

## 与仓库其他部分

| 本书 ch7 | 对照 |
|----------|------|
| 控制流 lowering | [CI clox ch17 发字节码](../../../01_Crafting-Interpreters/part03_clox/chapter17_compiling-expressions/README.md) · ch23 jumps |
| 数组/结构 layout | RFR [第 2 章 layout](../../../02-RFR/Chapter-02-Types/02-layout.md) |
| 别名/歧义 | RFR 第 10 章 · LLVM `noalias` |
| 调用形态 | [ch6 §5 链接约定](../chapter06_procedures/05-call-linkages.md) |
| 虚方法 | [CI clox ch28 Methods](../../../01_Crafting-Interpreters/part03_clox/chapter28_methods/README.md) |

---

## 逻辑脉络

存哪 → 算符 → 数据布局寻址 → 分支/循环/switch → call/OOP — **Part II 收官**。

---

## 速记

## 本章速记

```text
§1  能寄存器则寄存器 · 歧义/别名→必须内存 · 安全vs优化
§2  算术: 类型转换/硬件选择 · 布尔: 数值vs控制流 · 条件码
§3  数组: 行/列主序+地址公式 · 字符串 · struct对齐/offset
§4  if布局 · 循环header/latch · switch: 链/二分/跳转表
§5  call形态=ch6落地 · vtable dispatch · Part II 收官→ch8优化
```

---

## 三句背诵

1. **代码形态 = 同一源结构的 lowering 选型，锁死后续优化空间。**
2. **别名逼内存；布尔可物化可分支；switch 密则跳转表。**
3. **ch5～7：IR + 运行时 + 怎么译 — 然后才优化。**

---

## Part II 小结

| 章 | 关键词 |
|:--:|--------|
| 5 | AST/CFG/SSA/符号表 |
| 6 | AR/ABI/堆/GC |
| 7 | 存哪/算符/数组/控制流/call |

---

## 自测

- [ ] 什么是歧义值？对寄存器分配的影响？
- [ ] 布尔用数值 vs 控制流各适合什么？
- [ ] 行主序二维数组下标公式
- [ ] switch 三种实现策略
- [ ] 虚调用底层形态一句话

---

## 阅读进度

- [x] ch7 代码形态（Part II 完结）
- [ ] ch8 优化概述（Part III 开始）

