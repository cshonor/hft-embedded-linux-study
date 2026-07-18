# 条款 42：typename 和 class 在模板里的区别

## 本节讲什么

模板参数声明两者等价；嵌套依赖类型必须加 `typename` 告知编译器这是类型。

## 示例

```cpp
template<typename T>
void f(T x) { /* T 可以是 typename 或 class，此处等价 */ }
```
