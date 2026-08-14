# 第 6 章 · 语法分析 · §1 定义的分析

← [本章目录](./README.md) · 上一节：[00-overview.md](./00-overview.md) · 下一节：[02-statements.md](./02-statements.md)

---

## 程序整体结构

解析入口：**单文件** `compilation_unit`。

```text
compilation_unit
  ├── import_stmts    // import 声明列表（C♭ 替代 #include）
  └── top_defs        // 顶层定义列表
```

| 非终端 | 含义 |
|--------|------|
| **`compilation_unit`** | 一个 `.cb` 源文件 |
| **`import_stmts`** | 零条或多条 `import stdio;` 等 |
| **`top_defs`** | 函数、全局变量、struct、typedef… |

→ [ch2 `import`](../chapter02_cflat-cbc/01-cflat-language.md) 无预处理器。

---

## 顶层定义规则

| 规则 | 内容 |
|------|------|
| **`defun`** | **函数定义** |
| **`defvars`** | **变量定义**（可多条声明） |
| **`defstruct`** | **结构体 / 联合体** 定义 |
| **`typedef`** | **类型别名** |

```text
top_defs : ( defun | defvars | defstruct | typedef )*
```

各规则内部再展开：返回类型、参数列表、函数体 `block` 等 — 细节在 cbc `.jj` 与源码对照阅读。

---

## C♭ 与 C 的语法改良

C 的 **声明语法** 易混淆（`int *p, q` 中 `q` 不是指针）。

**C♭ 改良**：**数组、指针等修饰符后置** — **类型** 与 **变量名** 边界清晰。

```text
// 概念（C♭ 风格）
int  x;
int  y*;      // 指针：修饰符跟名字后
int  a[10];   // 数组：后置 [10]
```

| C 习惯 | C♭ |
|--------|-----|
| `int *p` 前置 `*` | 后置 `*` 记法 |
| 声明符与类型纠缠 | **类型名 + 名 + 后置修饰** 分开解析 |

**Parser 收益**：`defvars` 规则更简单，少歧义 — 与 ch5 **提公因子** 同向（声明可读性）。

---

## 与 ch7

本章规则 **只建树/匹配结构**；ch7 在产生式里加 **JavaCC action** 实例化 **`ast` 包** 节点。

---

## 自测

- [ ] `compilation_unit` 两大部分
- [ ] 四种 `top_defs` 种类
- [ ] C♭ 后置修饰相对 C 的好处
