# 第 13 章 · Inheritance（继承） · §13.3 调用超类方法（Calling Superclass Methods）

← [本章目录](./README.md) · 上一节：[02-a-superclass-local-variable.md](./02-a-superclass-local-variable.md) · 下一节：[04-conclusion.md](./04-conclusion.md)

---

```lox
super.cook();           // Expr.Super + method name
super.cook(arg);        // 带参数调用
```

| 组件 | 说明 |
|------|------|
| **AST** | **`Expr.Super`**：method 名 + 可选 arguments |
| **查找** | 从 **`super` 绑定的超类** 起找方法（跳过子类 override） |
| **`this` 绑定** | 调用超类方法时 **`this` 仍是当前实例** → 包装 **`LoxBoundMethod(this, superMethod)`** |

**Resolver 误用检查**：

| 非法 | 原因 |
|------|------|
| 类外 **`super`** | 无 **`super` 局部绑定 |
| 无超类的类内 **`super`** | 没有 **`superclass`** |
| （与 **`this`** 类似） | 解析期报错，非运行时才炸 |

**执行流程（示意）**：

```text
visitSuperExpr:
  superClass = environment.get("super")   // LoxClass
  method     = superClass.findMethod(name)
  bound      = LoxBoundMethod(thisInstance, method)
  bound.call(args...)
```

---
