# 第 13 章 · Inheritance（继承） · §13.1 超类与子类（Superclasses and Subclasses）

← [本章目录](./README.md) · 上一节：[00-overview.md](./00-overview.md) · 下一节：[02-a-superclass-local-variable.md](./02-a-superclass-local-variable.md)

---

```lox
class Doughnut {
  cook() { print "Fry"; }
}
class BostonCream < Doughnut {
  cook() {
    super.cook();
    print "Fill with cream";
  }
}
```

| 语法 | 说明 |
|------|------|
| **`class Sub < Super { ... }`** | `<` 表示继承（Lox 无 `extends`） |
| **`LoxClass`** | 增加 **`superclass`** 指针（可为 `null`） |
| 方法暴露 | 子类实例查找方法时：**本类 → 超类 → …** |

**执行 / 声明阶段**：

- 解析超类名 → 运行时取已定义的 **`LoxClass`**。
- 子类 **`LoxClass`** 保存 **`superclass` 引用**；实例方法查找沿链向上。

**与 ch12 字段优先**：实例 **fields** 仍优先于类方法；继承只扩展 **方法表查找链**。

---
