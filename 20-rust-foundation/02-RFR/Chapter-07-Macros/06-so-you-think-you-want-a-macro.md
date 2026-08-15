# 2.3 So You Think You Want a Macro（你真的需要宏吗）

> 所属：**Procedural Macros** · [← 章索引](./README.md)

RFR 强调的决策顺序 — **不要默认上宏**。总览 → [00 宏核心总览](./00-macros-overview.md) · 声明宏何时用 → [01](./01-when-to-use-declarative-macros.md)

## 选型 ladder

```text
1. 泛型 + trait + 普通函数  ── 能表达吗？→ 用
2. macro_rules!              ── 只是语法形状/样板？→ 用
3. 过程宏                    ── 需要解析/变换任意 AST？→ 用，并接受编译成本
```

| 需求 | 倾向 |
|------|------|
| 同一逻辑，不同类型 | **泛型** |
| `vec!` / 多分支 token 模式 | **声明宏** |
| `#[derive(...)]` / 属性 DSL | **过程宏** |
| 可读性 / 团队维护 | **少宏** — 宏是 API，错误信息要投资 |

## 反模式

- 用宏替代本可用 **泛型 + trait** 解决的类型差异
- 过程宏生成**巨量**代码而不测编译时间
- 不把错误**钉在用户写的 span** 上 → [07 如何工作](./07-how-procedural-macros-work.md)

## 什么时候必须用宏？

| 场景 | 示例 |
|------|------|
| 批量生成 `impl` | `#[derive(Serialize)]`、ORM 实体 |
| 自定义 DSL | `sqlx::query!`、路由属性 |
| 按字段元编程 | 泛型 + trait 无法覆盖的生成逻辑 |

→ 完整对比：[00-3 宏 vs 函数](./00-3-macro-vs-function.md)

→ 声明宏何时用 [01](./01-when-to-use-declarative-macros.md)
