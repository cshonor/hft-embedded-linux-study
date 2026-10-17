# 条款 53：留心编译器不同版本的差异

## 本节讲什么

不同 C++ 标准、不同编译器对条款实现细节有差异，不要绑定某一编译器特有行为。

## 示例

```cpp
#if __cplusplus >= 201103L
    // C++11 特性
#else
    // 回退实现
#endif
```

---

## 代码自测

**题目 1：** 以下编译器警告为什么不应忽视？
```cpp
class Widget {
    virtual ~Widget() {};
};
// g++ -Wall 警告: 'Widget' has virtual functions but non-virtual destructor
```

<details>
<summary>参考答案</summary>

如果 `Widget` 有虚函数，通常意味着它会被用作多态基类。非虚析构函数在 `delete base*` 时不调用派生类析构——资源泄漏。即使当前没有派生类，将来也大概率会有。应声明 `virtual ~Widget() = default;`。不同编译器的警告含义不同，应该理解每个警告的根因而非简单关闭。

</details>
