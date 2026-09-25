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

<details>
<summary>参考答案</summary>

1. 因为编译器生成的 `~Widget()`（以及 `unique_ptr<Impl>` 的删除器实例化）需要在析构点看到 `Impl` 的**完整定义**，才能调用 `Impl` 的析构函数。如果析构函数定义在头文件里并在类内隐式生成，那么所有包含该头文件的翻译单元在实例化析构时都会要求 `Impl` 完整——但头文件里 `Impl` 只是前向声明，于是编译失败（典型报错："incomplete type ... in destructor"）。把 `~Widget();` 声明在头文件、定义在 `.cpp`（Impl 定义之后），析构就只在 `.cpp` 里实例化一次，头文件可以完全不暴露 `Impl` 的定义。

2. `std::unique_ptr<T>` 的默认删除器要求 `sizeof(T)` 已知且 `T` 可析构：在 `unique_ptr` **析构、重置（`reset`）、移动到/自** 等会调用删除器的地方，`T` 必须是完整类型（complete type）。这是标准对 `unique_ptr` 的明确要求（`shared_ptr` 在这方面更宽松，它的删除器是在构造时绑定的类型擦除形式，析构时不必要求 `T` 完整）。Pimpl 因此要求：所有会触发删除器的特殊成员函数（析构、移动构造、移动赋值——如果它们在头文件中被隐式生成）都必须在 `Impl` 完整的翻译单元里定义。

3. Pimpl（pointer to implementation）本质就是用 C++ 的 RAII 智能指针实现 C 里的 **opaque pointer**（不透明指针）惯用法：C 里头文件只声明 `typedef struct Widget Widget;`，实现细节藏在 `.c` 里，调用方只拿到一个不完整类型的指针。二者目的一致——隐藏实现、稳定 ABI；区别是 C 版本需要手写 `Widget_create`/`Widget_destroy` 手动管理生命周期，而 C++ 的 Pimpl 用 `unique_ptr` 让编译器自动释放，异常路径也不会泄漏。

4. 头文件里不再 `#include` 实现类的依赖（如 `<vector>`、第三方库头、平台头），只保留一个前向声明 `struct Impl;` 和一个 `unique_ptr<Impl>`。这样：①实现变化（增删私有成员、换容器、换库）只重编译那一个 `.cpp`；②包含该头文件的所有翻译单元不再被这些依赖"传染"，增量编译范围从"全量"缩小到局部；③编译防火墙也带来 ABI 稳定——`Widget` 对象大小固定为一个指针，库升级不必重编译调用方。

</details>

---

## 参考与延伸

- 下一章：[第 5 章 右值引用、移动语义与完美转发](../ch05-rvalue-move-forwarding/README.md)
- 回到：[第 4 章 智能指针](README.md)
