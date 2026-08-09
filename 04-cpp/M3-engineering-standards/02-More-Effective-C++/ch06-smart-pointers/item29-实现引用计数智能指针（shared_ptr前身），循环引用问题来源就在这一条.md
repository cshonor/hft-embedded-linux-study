# 条款 29：实现引用计数智能指针（shared_ptr 前身），循环引用问题来源就在这一条

## 本节讲什么

> 待补充详细笔记（错误案例、原理、正确写法、代码示例）。

## 示例

```cpp
class RefCounted {
    int *count;
    int *data;
public:
    RefCounted() : count(new int(1)), data(new int(0)) {}
    RefCounted(const RefCounted &o) : count(o.count), data(o.data) { ++*count; }
    ~RefCounted() { if (--*count == 0) { delete count; delete data; } }
};
```

---

## 代码自测

**题目 1：** 引用计数智能指针的循环引用问题是什么？如何解决？
```cpp
class Node {
    std::shared_ptr<Node> next;
public:
    ~Node() { cout << "destroyed"; }
};
auto a = std::make_shared<Node>();
auto b = std::make_shared<Node>();
a->next = b;
b->next = a;  // 循环引用
// a 和 b 出作用域后，析构函数会被调用吗？
```

<details>
<summary>参考答案</summary>

不会调用。`a` 出作用域时引用计数从 2→1（b 的 next 还引用 a），`b` 出作用域时从 2→1（a 的 next 还引用 b）——两个对象都保持在引用计数 1，永远不会析构——内存泄漏。解决：用 `std::weak_ptr` 打破循环：
```cpp
class Node {
    std::shared_ptr<Node> next;
    std::weak_ptr<Node> prev;  // 弱引用不增加引用计数
};
```
或重构设计避免循环依赖。

</details>
