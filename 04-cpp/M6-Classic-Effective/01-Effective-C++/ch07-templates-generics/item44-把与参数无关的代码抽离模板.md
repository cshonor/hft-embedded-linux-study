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

---

## 代码自测

**题目 1：** 以下模板会产生代码膨胀，如何消除？
```cpp
template<typename T, int N>
class SquareMatrix {
    void invert() { /* N×N 矩阵求逆，与 T 无关的逻辑 */ }
};
SquareMatrix<double, 5> m1;
SquareMatrix<double, 10> m2;  // invert() 的代码被实例化两份
```

<details>
<summary>参考答案</summary>

将与 N 无关的逻辑抽到非模板基类或独立函数中：
```cpp
class SquareMatrixBase {
    void invert(int n) { /* 通用 NxN 求逆 */ }
};
template<typename T, int N>
class SquareMatrix : private SquareMatrixBase {
    void invert() { SquareMatrixBase::invert(N); }
};
```
这样 `invert` 的实现只有一份，N 作为参数传入。

</details>
