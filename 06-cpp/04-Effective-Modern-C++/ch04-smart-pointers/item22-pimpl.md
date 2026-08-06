# Item 22：用 Pimpl 惯用法降低编译依赖

> 第 4 章 智能指针 · Item 22 · 上一节：[Item 21 make 函数](item21-make-functions.md)

## 这节讲什么

Pimpl（Pointer to Implementation）把实现细节藏到 `.cpp`，头文件只留一个 `unique_ptr<Impl>`——降低编译依赖，加速增量编译。

---

## 核心结构

```cpp
// widget.h
class Widget {
    struct Impl;                    // 前向声明，不需要完整定义
    std::unique_ptr<Impl> pImpl;    // 只需要 Impl 的指针
public:
    Widget();
    ~Widget();   // 必须在 .cpp 定义（unique_ptr 析构需要完整类型）
    Widget(Widget&&) noexcept;
    Widget& operator=(Widget&&) noexcept;
};

// widget.cpp
#include "widget.h"
struct Widget::Impl {               // 实现细节藏在 .cpp
    int data;
    std::vector<int> vec;
    void doWork();
};
Widget::Widget() : pImpl(std::make_unique<Impl>()) {}
Widget::~Widget() = default;        // 在 .cpp 里，Impl 已完整定义
```

---

## 新手要点（和 C 的区别）

- **C 的 opaque pointer**：C 里也有类似手法——头文件只声明 `struct Widget;`，`.c` 里定义 `struct Widget { ... }`。《C 和指针》ch07 讲过这个。C++ 版用 `unique_ptr` 自动管理释放。
- **为什么析构要在 .cpp**：`unique_ptr` 析构需要调用 `delete Impl`，但 `delete` 需要 `Impl` 的完整定义。头文件里只有前向声明，所以析构必须放 `.cpp`。

---

## HFT 关联

- **降低编译依赖**：大型交易引擎头文件改动会触发全量重编译。Pimpl 把核心结构藏到 `.cpp`，增量编译从分钟级降到秒级。

---

## 自测题

1. Pimpl 里为什么析构函数必须在 `.cpp` 而非头文件定义？
2. `unique_ptr` 析构对类型完整性的要求是什么？
3. Pimpl 和 C 的 opaque pointer 有什么关系？
4. Pimpl 如何降低编译依赖？

---

## 参考与延伸

- 下一章：[第 5 章 右值引用、移动语义与完美转发](../ch05-rvalue-move-forwarding/README.md)
- 回到：[第 4 章 智能指针](README.md)
