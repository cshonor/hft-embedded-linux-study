# 第 9 章 · 语义分析（1）引用的消解 · §3 类型名称的消解

← [本章目录](./README.md) · 上一节：[02-variable-resolution.md](./02-variable-resolution.md) · 下一节：---

## 核心目的

解析前类型以 **`TypeRef`**（**类型名称**）出现 — [ch8 设计](../chapter08_build-ast/01-expr-ast.md)。

| 消解前 | 消解后 |
|--------|--------|
| **`TypeRef`** — 字符串级名字 | **`Type`** — 编译器内部 **类型实体** |

**类型名称消解** = 把语法上的「名字」换成 **可操作的 Type 对象**。

---

## TypeResolver 与 TypeTable

**`TypeResolver`** 负责 — **不用 Scope 栈**：

| 原因 | 说明 |
|------|------|
| C♭ 类型 **无嵌套作用域** | 类型名在 **单一** 类型命名空间 |
| **TypeTable** 足够 | `TypeRef` → `Type` 映射表 |

```text
TypeTable
  "int"    → IntType
  "Point"  → StructType(…)
  …
```

**注册**：`struct`/`typedef` 定义阶段填入表；**引用**阶段查表替换。

---

## AST 遍历与替换

`TypeResolver` 访问 **所有带 TypeRef 的节点**，查 **`TypeTable`**，**写入 `Type`**：

| 节点位置（概念） | 转换 |
|------------------|------|
| **变量定义** | 字段类型 Ref → Type |
| **函数定义** | 返回类型、**形参** 类型 |
| **类型转换** | `(TypeRef) expr` |
| **sizeof 等** | 涉及类型的字面/运算符 |

```text
visit(DefinedFunction f) {
  f.returnType = table.resolve(f.returnTypeRef);
  for (param : f.params)
    param.type = table.resolve(param.typeRef);
}
```

**替换策略**：节点上 **并存 Ref 与 Type 字段**，或 Ref 解析后置 null — 以实现为准；语义后下游 Pass 只读 **`Type`**。

---

## 与变量消解的对比

| | 变量 `LocalResolver` | 类型 `TypeResolver` |
|---|---------------------|---------------------|
| **结构** | Scope **树/栈** | **扁平 TypeTable** |
| **嵌套** | 块作用域 | 无 |
| **失败** | 未定义变量 | 未知类型名 |

**顺序**：书中 **先变量、后类型名** — 部分节点可能同时依赖两者；五阶段表保证依赖。

---

## 与 ch10

类型实体就绪后 → **类型定义是否合法**、**表达式类型**、**TypeChecker** — ch10。

---

## 自测

- [ ] TypeRef 与 Type 各在何时存在
- [ ] 为何类型不用 Scope 栈
- [ ] TypeTable 在哪类节点上查表
