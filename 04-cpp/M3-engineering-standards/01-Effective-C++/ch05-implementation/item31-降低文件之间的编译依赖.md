# 条款 31：降低文件之间的编译依赖

## 本节讲什么

依赖头文件前置声明，用指针/引用代替完整类定义；PIMPL 指针封装实现，头文件只留接口。

**有源前置声明**：`.h` 里 `class Widget;` 与「实现在已链入的 `Widget.cpp` / 库中」配套——不是空占位，而是编译期减依赖、链接期有实现（HFT 大项目常用）。

## 示例

```cpp
// 头文件尽量少 #include，多用前置声明
class Observer;
class Subject {
    std::vector<std::unique_ptr<Observer>> obs;
};
```

---

## 代码自测

**题目 1：** Pimpl 惯用法如何降低编译依赖？
```cpp
// widget.h
class WidgetImpl;  // 前置声明
class Widget {
    WidgetImpl* pImpl;
public:
    Widget();
    ~Widget();
    void doSomething();
};
```

<details>
<summary>参考答案</summary>

Pimpl（Pointer to Implementation）将实现细节移到 .cpp 文件中。`widget.h` 只需要 `WidgetImpl` 的前置声明，不 include `WidgetImpl` 的完整定义。当 `WidgetImpl` 的成员变化时，只需重编译 `widget.cpp`，所有 include `widget.h` 的文件不受影响。注意：析构函数必须在 .cpp 中定义（因为需要完整类型来 delete pImpl）。C++11 后用 `std::unique_ptr<WidgetImpl>` 替代裸指针。

</details>
