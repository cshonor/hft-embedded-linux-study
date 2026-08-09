# 条款 42：typename 和 class 在模板里的区别

## 本节讲什么

模板参数声明两者等价；嵌套依赖类型必须加 `typename` 告知编译器这是类型。

## 示例

```cpp
template<typename T>
void f(T x) { /* T 可以是 typename 或 class，此处等价 */ }
```

---

## 代码自测

**题目 1：** 下面代码中 `typename` 的作用是什么？去掉会怎样？
```cpp
template<typename T>
void f() {
    typename T::iterator it;  // typename 的作用？
    // T::iterator it;  // 不加 typename 会怎样？
}
```

<details>
<summary>参考答案</summary>

`typename` 告诉编译器 `T::iterator` 是一个类型名，不是变量名。去掉 `typename`，编译器默认假设 `T::iterator` 是变量（因为 `T` 未知时无法确定），所以 `T::iterator it;` 会被解析为变量乘变量——编译错误。规则：模板中依赖模板参数的嵌套类型名前必须加 `typename`（C++20 后可省略，但之前的版本必须加）。

</details>
