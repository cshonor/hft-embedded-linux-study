# 条款 8：吃透 new、delete 多种重载形式的不同含义（全局/类专属/placement new）

## 本节讲什么

> 待补充详细笔记（错误案例、原理、正确写法、代码示例）。

## 示例

```cpp
void *p1 = ::operator new(100);       // 全局
void *p2 = ::operator new(100, buf);  // placement
Widget *w = new Widget;               // 类专属（若定义）
```

---

## 代码自测

**题目 1：** 以下四种 new 各是什么含义？
```cpp
// 1
Widget* p = new Widget;
// 2
void* mem = ::operator new(sizeof(Widget));
new(mem) Widget;  // 这是什么 new？
// 3
class Widget {
    static void* operator new(size_t, ostream& log);  // 这是什么？
};
// 4
new(log) Widget;  // 用哪个？
```

<details>
<summary>参考答案</summary>

1. 普通 new（operator new 分配 + 构造函数）
2. placement new（在已有内存上构造对象，不分配）
3. 自定义 placement new（带额外参数的 operator new 重载）
4. 调用 3 中定义的带 log 参数的 placement new
`operator new` 只负责分配内存，`new` 表达式还会调用构造函数。`delete` 对应 `operator delete` + 析构。

</details>
