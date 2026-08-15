# 2.2 The Cost of Procedural Macros（过程宏的代价）

> 所属：**Procedural Macros** · [← 章索引](./README.md)

## 编译时间

| 来源 | 影响 |
|------|------|
| **proc-macro crate 依赖** | `syn` + `quote` 等显著增加**宏 crate 自身**编译负担 |
| **生成代码体积** | 不慎生成大量单态 / 冗长 `impl` → 拖慢**下游 crate** 编译与优化 |

## 实践

- 日常重复 → 优先 **`macro_rules!`**
- 框架 / 强 DSL 才上过程宏，并**主动控制**生成代码量
- 用 **`cargo-expand`** 查看展开结果 → [第 13 章 tools](../Chapter-13-Rust-Ecosystem/01-tools.md)

→ 选型 [06 你真的需要宏吗](./06-so-you-think-you-want-a-macro.md)
