# 条款 30：透彻理解内联 inline 的优缺点

## 本节讲什么

小高频函数适合 inline；大函数、递归、虚函数不要 inline，代码膨胀得不偿失。

## 示例

```cpp
// Widget.h 只前置声明，实现放 Widget.cpp
class WidgetImpl;
class Widget {
    std::unique_ptr<WidgetImpl> pImpl;
};
```

---

## 代码自测

**题目 1：** 下面两种 inline 方式有什么区别？
```cpp
// 方式A：隐式 inline（类内定义）
class Widget {
    int size() const { return n; }
};
// 方式B：显式 inline
inline int Widget::size() const { return n; }
```

<details>
<summary>参考答案</summary>

功能等价，都是建议编译器内联。但过度 inline 会增加代码体积（code bloat）——如果函数太大或被多处调用，内联展开会显著增大二进制体积，反而降低指令缓存命中率。通常只 inline 小的、频繁调用的函数（1-3 行）。构造/析构函数看似空，实际可能有大量隐含代码（成员构造/析构），不宜 inline。

</details>
