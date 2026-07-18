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
