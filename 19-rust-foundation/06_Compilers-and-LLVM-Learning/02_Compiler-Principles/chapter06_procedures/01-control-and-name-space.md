# 第 6 章 · 过程抽象 · §1 控制抽象与名字空间

← [本章目录](./README.md) · 上一节：[00-overview.md](./00-overview.md) · 下一节：[02-activation-records.md](./02-activation-records.md)

---

## 控制抽象（Control Abstraction）

**过程（Procedure / Function）** — 封装代码块，可**多次调用**。

编译器必须实现：

| 机制 | 说明 |
|------|------|
| **Call** | 跳转到被调过程，保存返回点 |
| **Return** | 恢复调用者执行流 |

→ 所有有函数的语言（C / Rust / Lox / Java）的共同底座。

**CI clox**：`OP_CALL` + **CallFrame** → [ch24](../../../01_Crafting-Interpreters/part03_clox/chapter24_calling-and-closures/README.md)

---

## 名字空间与作用域

编译器维护**变量可见性**规则。

| 模型 | 例子 | 特点 |
|------|------|------|
| **词法作用域（Lexical / Static）** | C、Pascal、Rust、Lox | 嵌套块可**遮蔽**同名；编译期用**静态坐标**定位 |

→ [ch5 符号表](../chapter05_ir/07-symbol-tables.md) · [jlox ch11 Resolver](../../../01_Crafting-Interpreters/part02_jlox/chapter11_resolving-and-binding/README.md)

---

## 面向对象语言的名字空间

| 话题 | 做法 |
|------|------|
| **对象** | 表示为 **object record**（字段 + 方法表等） |
| **类层次 / 继承** | 字段布局、方法查找链 |
| **分发（Dispatch）** | 虚方法 / vtable — 解析「调哪个方法」 |

**Rust**：无传统继承；`impl` + trait 对象 `dyn` 有类似 dispatch 问题 → RFR 第 2 章 dispatch。

**C++ / Java**：本章 OOP 名字空间与 ch11 后端 **虚表** 生成相关。

---

## 与 ch4 分工

| ch4 | ch6 |
|-----|-----|
| **静态**语义 — 类型、符号表 | **动态**布局 — 调用时栈帧、传参、返回 |
