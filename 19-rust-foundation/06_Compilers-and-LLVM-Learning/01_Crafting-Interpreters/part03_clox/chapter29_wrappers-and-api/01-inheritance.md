# 第 29 章 · Superclasses（超类与继承） · §29.1 继承（Inheritance）

← [本章目录](./README.md) · 上一节：[00-overview.md](./00-overview.md) · 下一节：[02-a-superclass-local-variable.md](./02-a-superclass-local-variable.md)

---

```lox
class Doughnut { cook() { print "Fry"; } }
class BostonCream < Doughnut {
  cook() { super.cook(); print "Cream"; }
}
```

**编译**：**`class BostonCream < Doughnut`** → 超类名常量 + **`OP_SUBCLASS`** / **`OP_INHERIT`** 序列（以书为准）。

**Copy-down inheritance（向下拷贝）**：

```text
OP_INHERIT:
  将 superclass.methods 表中所有条目
  复制到 subclass.methods（子类未 override 的键）
```

| 优点 | 说明 |
|------|------|
| **运行时查找 O(1) 一次 hash** | 无需沿继承链 walk |
| **override** | 子类声明同名方法 → **覆盖** 拷贝项 |

| 代价 | 说明 |
|------|------|
| **声明时拷贝** | 超类后改方法 **不影响** 已声明子类（Lox 语义可接受） |
| **内存** | 方法闭包 **共享引用** · 非 deep copy chunk |

**对照 jlox [ch13](../../part02_jlox/chapter13_inheritance/README.md)**：运行时 **超类链查找** vs clox **表拷贝** —— 典型 **编译期换运行时** 优化。

---
