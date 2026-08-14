# Item 28 · 核心知识点

← [Item 28 目录](./README.md)

### 元编程（Metaprogramming）

- **写生成代码的代码**；宏用于消除**确定、重复**的样板。

### 宏的类型

| 类别 | 形式 | 说明 |
|------|------|------|
| **声明式** | `macro_rules!` | macros by example；按 token 角色匹配插入 |
| **过程宏** | 独立 `proc-macro` crate | 操作 **TokenStream** |

过程宏三种：

1. **类函数** — `foo!(...)`
2. **属性** — `#[foo]`
3. **派生** — `#[derive(Foo)]`（**最常用**）

### 卫生性（Hygienic）

- 声明式宏展开**不会**意外捕获调用点局部变量 → 避免 C 宏式命名冲突。

---
