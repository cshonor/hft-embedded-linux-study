# 第六章 继承与面向对象设计

共 9 条条款。

## 条款

- [条款 32：确定你的 public 继承塑模出 is-a 关系](./item32-确定你的public继承塑模出is-a关系.md)
- [条款 33：区分接口继承和实现继承](./item33-区分接口继承和实现继承.md)
- [条款 34：不要重写继承而来的非虚成员函数](./item34-不要重写继承而来的非虚成员函数.md)
- [条款 35：不要重写继承而来的默认参数值](./item35-不要重写继承而来的默认参数值.md)
- [条款 36：避免多层继承里出现名字遮蔽](./item36-避免多层继承里出现名字遮蔽.md)
- [条款 37：绝不重新定义继承而来的缺省参数值](./item37-绝不重新定义继承而来的缺省参数值.md)
- [条款 38：组合（复合）关系优先于继承](./item38-组合（复合）关系优先于继承.md)
- [条款 39：谨慎使用私有继承](./item39-谨慎使用私有继承.md)
- [条款 40：谨慎使用多重继承](./item40-谨慎使用多重继承.md)


## 章节摘要

继承与 OOP：public 继承是 is-a、接口继承 vs 实现继承、不重写非虚函数、不重写默认参数值、避免名字遮蔽、组合优于继承、谨慎私有继承、谨慎多重继承。

## 代码自测

### Q1: is-a 关系

```cpp
class Bird {
public:
    virtual void fly() { std::cout << "flying "; }
};
class Penguin : public Bird {
    // 企鹅不会飞！
};
Penguin p;
p.fly();  // 输出什么？设计有问题吗？
```

> `p.fly()` 会怎样？`Penguin` 继承 `Bird` 合理吗？

<details>
<summary>答案与复习指引</summary>

**`p.fly()` 输出 "flying "——但企鹅不会飞！** 设计有误。

**问题：** `public` 继承表示 is-a 关系——"企鹅是一种鸟"在自然语言中成立，但在 C++ 模型中 "鸟能飞" 对企鹅不适用。继承意味着派生类能做基类能做的一切。

**修复方案：**
1. 基类不定义 `fly()`，只定义所有鸟都有的行为；`fly()` 放到 `FlyingBird` 子类
2. `Penguin` 重写 `fly()` 抛异常（运行时错误，不优雅）
3. 不让 `Penguin` 继承 `Bird`（如果 `Bird` 定义了 `fly`）

**教训：** `public` 继承必须满足 "派生类 is-a 基类"——所有能对基类做的事都能对派生类做。

**复习：** → [条款 32：确定你的 public 继承塑模出 is-a 关系](./item32-确定你的public继承塑模出is-a关系.md)
</details>

### Q2: 默认参数值不被重写

```cpp
class Base {
public:
    virtual void draw(int color = 1) { std::cout << "Base " << color; }
};
class Derived : public Base {
public:
    void draw(int color = 2) override { std::cout << "Derived " << color; }
};
Base *p = new Derived;
p->draw();  // 输出什么？
```

> `p->draw()` 输出什么？默认参数值用的是基类还是派生类的？

<details>
<summary>答案与复习指引</summary>

**输出 "Derived 1"。** 虚函数是动态绑定的（调 `Derived::draw`），但默认参数值是**静态绑定**的（用 `Base` 的 `color=1`）。

**原因：** 默认参数值在编译期确定（基于指针/引用的静态类型）。如果运行时绑定默认参数，编译器无法高效实现（虚函数表存函数指针，但默认参数在调用点注入）。

**教训：** 不要重写继承而来的默认参数值——要么在基类定义默认值且不在派生类改它，要么用 NVI（Non-Virtual Interface）惯用法把默认参数放到非虚函数里。

**复习：** → [条款 35：不要重写继承而来的默认参数值](./item35-不要重写继承而来的默认参数值.md)
</details>

### Q3: 组合优于继承

```cpp
// 方案 A: 继承
class Stack : public std::vector<int> {
public:
    void push(int x) { push_back(x); }
    int pop() { int x = back(); pop_back(); return x; }
};
// 方案 B: 组合
class Stack {
    std::vector<int> impl;
public:
    void push(int x) { impl.push_back(x); }
    int pop() { int x = impl.back(); impl.pop_back(); return x; }
};
```

> A 和 B 哪个更好？为什么？

<details>
<summary>答案与复习指引</summary>

**B 更好。**

**A（继承）问题：**
1. `Stack` 继承 `vector` 后暴露了 `vector` 的所有接口（`insert`/`erase`/`operator[]`），破坏了栈的 LIFO 语义
2. `vector` 无虚析构——通过 `Stack*` 删除 `vector` 部分 UB
3. `Stack` 和 `vector` 不是 is-a 关系（栈不是"一种向量"）

**B（组合）优势：**
1. 只暴露需要的接口（`push`/`pop`），封装完整
2. 不依赖 `vector` 的实现细节，可换 `deque`/`list`
3. "Stack has-a vector" 是正确的语义

**规则：** 优先组合（has-a）而非继承（is-a）。只有在真正的 is-a 关系时才用 `public` 继承。

**复习：** → [条款 38：组合（复合）关系优先于继承](./item38-组合（复合）关系优先于继承.md)
</details>
