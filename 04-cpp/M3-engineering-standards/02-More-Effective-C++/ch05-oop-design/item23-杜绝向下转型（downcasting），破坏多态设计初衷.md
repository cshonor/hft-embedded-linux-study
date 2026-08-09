# 条款 23：杜绝向下转型（downcasting），破坏多态设计初衷

## 本节讲什么

> 待补充详细笔记（错误案例、原理、正确写法、代码示例）。

## 示例

```cpp
void process(Base &b) {
    // Derived *d = static_cast<Derived*>(&b);  // 避免向下转型
    b.virtualMethod();
}
```

---

## 代码自测

**题目 1：** 以下向下转型有什么风险？
```cpp
Base* p = getShape();
if (p->type() == CIRCLE) {
    Circle* c = static_cast<Circle*>(p);
    c->setRadius(10);
}
```

<details>
<summary>参考答案</summary>

风险：1) 如果 `p` 的实际类型不是 Circle，`static_cast` 不检查，行为未定义；2) 每加一种新形状需要修改所有 if-else 链，违反开闭原则；3) 类型判断分散在代码各处，维护困难。正确做法：用虚函数实现多态——`p->setSize(10)`，每种形状自己 override `setSize`。消除向下转型 = 用多态替代类型 switch。

</details>
