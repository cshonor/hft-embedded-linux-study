# 条款 34：C++ 和 C 混合编程的规范、链接、命名修饰、库兼容避坑

## 本节讲什么

> 待补充详细笔记（错误案例、原理、正确写法、代码示例）。

## 示例

```cpp
extern "C" void c_api(int x);
// C++ 实现
extern "C" void c_api(int x) { /* ... */ }
```

---

## 代码自测

**题目 1：** C 和 C++ 混合编程时，以下代码有什么问题？
```cpp
// C++ 代码
class Widget { ... };
extern "C" void process(Widget* w);  // 正确吗？
```

<details>
<summary>参考答案</summary>

问题：`extern "C"` 禁用名称修饰（name mangling），但 C++ 类的成员函数和模板无法在不修饰的名称下工作。`process` 的参数是 `Widget*`——C 编译器不认识 C++ 类。正确做法：1) C++ 侧用 `extern "C"` 暴露 C 兼容的接口（只用 POD 类型参数）；2) C 侧用不透明指针（`void*`）传递 C++ 对象：
```cpp
// C++ 侧
extern "C" void* widget_create() { return new Widget; }
extern "C" void widget_destroy(void* w) { delete static_cast<Widget*>(w); }
// C 侧
void* w = widget_create();
widget_destroy(w);
```

</details>
