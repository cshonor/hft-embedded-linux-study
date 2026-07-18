# 条款 44：把与参数无关的代码抽离模板

## 本节讲什么

避免模板实例化产生大量重复代码，代码膨胀；公共逻辑抽成非模板基类/全局函数。

## 示例

```cpp
template<typename T>
class SquareMatrixBase {
protected:
    void invert(std::size_t size);  // 与 T 无关的代码抽离
};
template<typename T>
class SquareMatrix : private SquareMatrixBase<T> { /* ... */ };
```
