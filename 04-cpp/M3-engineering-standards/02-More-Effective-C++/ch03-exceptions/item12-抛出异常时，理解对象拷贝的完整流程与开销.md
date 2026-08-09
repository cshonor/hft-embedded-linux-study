# 条款 12：抛出异常时，理解对象拷贝的完整流程与开销

## 本节讲什么

> 待补充详细笔记（错误案例、原理、正确写法、代码示例）。

## 示例

```cpp
class MyError : public std::exception {};
void f() {
    try { throw MyError(); }
    catch (MyError e) { /* 按值捕获会切片/拷贝 */ }
}
```

---

## 代码自测

**题目 1：** catch 子句和函数参数有什么区别？
```cpp
void f(Widget w);          // 按值传参
catch (Widget w) { ... }   // 按值捕获异常
```
两者都会拷贝对象吗？

<details>
<summary>参考答案</summary>

都会拷贝，但异常对象的拷贝可能更多次：1) throw 表达式构造临时对象（第1次拷贝）；2) 异常对象被拷贝到 catch 子句（第2次拷贝）。而且异常对象在传播过程中可能被拷贝多次。函数参数最多拷贝1次。此外，catch 挕获时不支持隐式转换（除了基类→派生类之间的类层次转换）。推荐 catch by reference：`catch (const Widget& w)`。

</details>
