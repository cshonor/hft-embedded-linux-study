# 条款 19：理解临时对象、拷贝构造、返回值优化 RVO/NRVO

## 本节讲什么

> 待补充详细笔记（错误案例、原理、正确写法、代码示例）。

## 示例

```cpp
BigObject factory() {
    return BigObject();  // 返回值优化 RVO
}
BigObject o = factory();
```

---

## 代码自测

**题目 1：** 以下函数返回时会发生什么？RVO 如何优化？
```cpp
Widget createWidget() {
    Widget w;
    // ... 修改 w ...
    return w;  // 会拷贝吗？
}
Widget result = createWidget();  // 总共几次拷贝？
```

<details>
<summary>参考答案</summary>

无优化时：1) `w` 拷贝到返回值临时对象；2) 临时对象拷贝到 `result`——2 次拷贝。
RVO（Return Value Optimization）：编译器直接在 `result` 的位置构造 `w`，省去 2 次拷贝——0 次拷贝。
NRVO（Named RVO）：对具名局部变量 `w` 也做类似优化。
C++17 起对纯右值返回强制 RVO（guaranteed copy elision）。但要注意：如果返回的是函数参数或不同分支返回不同变量，RVO 可能不生效。

</details>
