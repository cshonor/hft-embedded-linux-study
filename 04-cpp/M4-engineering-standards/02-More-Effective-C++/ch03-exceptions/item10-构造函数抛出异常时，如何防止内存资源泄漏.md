# 条款 10：构造函数抛出异常时，如何防止内存/资源泄漏

## 本节讲什么

> 待补充详细笔记（错误案例、原理、正确写法、代码示例）。

## 示例

```cpp
class Widget {
    int *data;
public:
    Widget() : data(new int[100]) {
        if (fail()) throw std::runtime_error("ctor failed");
        // 已分配内存需在 catch 或成员析构中清理
    }
    ~Widget() { delete[] data; }
};
```

---

## 代码自测

**题目 1：** 构造函数中 `new` 了两个对象，如果第二个 `new` 抛异常怎么办？
```cpp
class Widget {
    Foo* a;
    Bar* b;
public:
    Widget() : a(new Foo), b(new Bar) {}  // 如果 new Bar 抛异常？
    ~Widget() { delete a; delete b; }
};
```

<details>
<summary>参考答案</summary>

`new Bar` 抛异常时，`a` 已分配但析构函数不会被调用（构造未完成，对象不存在）——`a` 泄漏。解法1：用 try-catch 在构造函数中清理。解法2（推荐）：用智能指针成员：
```cpp
class Widget {
    std::unique_ptr<Foo> a;
    std::unique_ptr<Bar> b;
public:
    Widget() : a(std::make_unique<Foo>()), b(std::make_unique<Bar>()) {}
};
```
如果 `make_unique<Bar>()` 抛异常，`a` 的析构函数会自动释放——因为 `a` 已经构造完成。

</details>
