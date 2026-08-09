# 第五部分 面向对象高级设计（Object-Oriented Design）

继承体系、接口设计、LSP、类层级设计。

## 条款

- [条款 21：按需把函数声明为虚函数，不要盲目虚函数](./item21-按需把函数声明为虚函数，不要盲目虚函数.md)
- [条款 22：区分接口继承和实现继承，很多继承设计错误根源就在这里](./item22-区分接口继承和实现继承，很多继承设计错误根源就在这里.md)
- [条款 23：杜绝向下转型（downcasting），破坏多态设计初衷](./item23-杜绝向下转型（downcasting），破坏多态设计初衷.md)
- [条款 24：理解虚函数、多重继承带来的内存布局、开销、歧义问题](./item24-理解虚函数、多重继承带来的内存布局、开销、歧义问题.md)
- [条款 25：虚拟继承（virtual public）的底层实现、巨大开销，能不用就不用](./item25-虚拟继承（virtualpublic）的底层实现、巨大开销，能不用就不用.md)
- [条款 26：限制某个类只能在堆上创建 / 只能在栈上创建的设计技巧](./item26-限制某个类只能在堆上创建只能在栈上创建的设计技巧.md)
- [条款 27：剖析运行时类型识别 RTTI（dynamic_cast/typeid）的开销与合理使用场景](./item27-剖析运行时类型识别RTTI（dynamic_casttypeid）的开销与合理使用场景.md)


## 章节摘要

OOP 高级设计：按需虚函数、接口继承 vs 实现继承、杜绝向下转型、多重继承布局开销、虚拟继承代价、限制堆/栈创建、RTTI 开销。

## 代码自测

### Q1: 向下转型的问题

```cpp
class Shape { public: virtual void draw() {} };
class Circle : public Shape { public: void drawCircle() {} };
void process(Shape *s) {
    Circle *c = static_cast<Circle*>(s);  // 向下转型
    c->drawCircle();
}
Shape *s = new Shape;  // 不是 Circle！
process(s);  // 会怎样？
```

> `process(s)` 传入非 `Circle` 对象会怎样？如何修复？

<details>
<summary>答案与复习指引</summary>

**UB。** `static_cast` 不做运行时检查——`s` 实际是 `Shape` 不是 `Circle`，`c->drawCircle()` 访问不存在的成员 → UB（可能崩溃）。

**修复方案：**
1. **`dynamic_cast`（运行时检查）：** `Circle *c = dynamic_cast<Circle*>(s); if (c) c->drawCircle();` — 但有 RTTI 开销
2. **虚函数（最佳）：** 在 `Shape` 中定义 `virtual void draw()`，`Circle` 重写。调用 `s->draw()` 自动分派到正确版本——无需转型
3. **Visitor 模式：** 不修改基类的情况下添加操作

**规则：** 向下转型是多态设计的坏味道——通常意味着应该用虚函数。如果必须转型，用 `dynamic_cast` 保证安全。

**复习：** → [条款 23：杜绝向下转型](./item23-杜绝向下转型（downcasting），破坏多态设计初衷.md)
</details>

### Q2: 虚函数开销

```cpp
class Handler {
public:
    virtual void process(int x) = 0;  // 纯虚函数
};
class FastHandler : public Handler {
public:
    void process(int x) override { /* 快速处理 */ }
};
// 在热循环中
for (int i = 0; i < 1000000; ++i) {
    handler->process(data[i]);  // 每次虚函数调用
}
```

> 每次虚函数调用比普通函数多什么开销？HFT 如何避免？

<details>
<summary>答案与复习指引</summary>

**虚函数开销：**
1. **间接调用**：经 vptr → vtable[slot] → call，多一次访存（vtable 可能 cache miss）
2. **不可内联**：编译器不知道运行时调哪个版本，无法内联
3. **分支预测**：间接跳转可能预测失败

**HFT 替代方案：**
1. **CRTP（静态多态）**：`template<class D> struct Base { void f() { static_cast<D*>(this)->impl(); } };` — 编译期分派，零开销+可内联
2. **`enum` + `switch`**：用标签代替虚函数，`switch` 可被优化为跳转表
3. **函数指针数组**：手动维护分派表，比虚函数更可控

**教训：** 热路径上的策略分派用 CRTP 或 `switch` 替代虚函数。非热路径用虚函数没问题。

**复习：** → [条款 20：按需选用静态绑定/动态绑定](./item20-按需选用静态绑定动态绑定（虚函数），不要无脑虚函数增加开销.md)
</details>
