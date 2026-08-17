# 条款 25：考虑提供不抛异常的 swap 重载

## 本节讲什么

自定义高效 `swap`，配合 ADL 查找，STL 容器、标准算法会优先调用你优化后的交换逻辑。

## 示例

```cpp
namespace std {
    template<> void swap(MyClass &a, MyClass &b) noexcept {
        a.swap(b);  // 不抛异常的 swap 重载
    }
}
```

---

## 代码自测

**题目 1：** 为什么 std::swap 对自定义类型可能效率不高？如何解决？
```cpp
class WidgetImpl { int data[1000]; /* ... */ };
class Widget { WidgetImpl* pImpl; };
```

<details>
<summary>参考答案</summary>

`std::swap` 的通用版本会创建临时副本并三次拷贝——对于 `Widget`（内部有 `WidgetImpl` 数组）代价很大。解法：特化 `swap` 只交换指针：
```cpp
namespace std {
    template<>
    void swap<Widget>(Widget& a, Widget& b) noexcept {
        std::swap(a.pImpl, b.pImpl);  // 只交换指针
    }
}
```
注意：`std::swap` 特化不应抛异常，声明 `noexcept`。

</details>
