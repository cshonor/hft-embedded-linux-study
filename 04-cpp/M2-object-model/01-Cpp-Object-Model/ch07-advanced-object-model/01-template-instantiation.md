# 7.1 模板实例化

> 第 7 章 高级对象模型 · 上一节：[本章导读](README.md) · 下一节：[7.2 异常处理实现](02-exception-impl.md)

## 这节讲什么

模板定义是"配方"，实例化时编译器按具体类型生成代码。代价是代码膨胀——每种类型一份代码。`extern template` 可缓解重复实例化。

---

## 为什么要学这个（先建立直觉）

C 程序员用 `void*` + 宏实现泛型，代码只有一份：

```c
// C：void* 泛型，一份代码
int cmp_int(const void* a, const void* b) {
    return *(int*)a - *(int*)b;
}
qsort(arr, n, sizeof(int), cmp_int);  // 一份 qsort 代码

// 或用宏（文本替换）
#define MAX(T, a, b) (((a) > (b)) ? (a) : (b))
MAX(int, 3, 4);    // 宏展开
MAX(double, 3.0, 4.0);  // 宏展开
```

C++ 的模板为每种类型生成独立代码——类型安全但代码膨胀：

```cpp
template<class T> T max(T a, T b) { return a > b ? a : b; }
max(3, 4);         // 生成 max<int> 的一份代码
max(3.0, 4.0);     // 生成 max<double> 的另一份代码
// 两份独立的机器码 → 二进制膨胀
```

---

## 实例化机制详解

### 隐式实例化

```cpp
template<class T>
class Vector {
    T* data;
    size_t size;
public:
    void push_back(const T& val);
    T& operator[](size_t i);
};

Vector<int> vi;     // 使用点自动实例化 Vector<int>
Vector<double> vd;  // 自动实例化 Vector<double>
// 每种类型一份独立代码
```

### 显式实例化

```cpp
// 在某个 .cpp 中集中实例化
template class Vector<int>;       // 生成 Vector<int> 的所有成员代码
template class Vector<double>;    // 生成 Vector<double> 的所有成员代码
// 其他翻译单元只需声明，不重复生成
```

### extern template

```cpp
// header.h
template<class T> class Vector { /* ... */ };
extern template class Vector<int>;  // 声明：别在这里实例化

// vector_int.cpp
template class Vector<int>;  // 只在这里实例化一次
// 其他 .cpp 包含 header.h 后不会重复实例化
```

### COMDAT 折叠

```
多个翻译单元都实例化了 Vector<int> → 链接器合并为一份（COMDAT 折叠）
但这增加了编译时间和对象文件体积
extern template 可以避免重复生成
```

---

## 常见错误（新手踩坑）

### 错误 1：过度模板化导致代码膨胀

```cpp
template<class T1, class T2, class T3>
class Triple { /* ... */ };
// 使用：Triple<int, double, char> t1;
//       Triple<int, double, int> t2;
//       Triple<long, double, char> t3;
// 每种组合一份代码 → 二进制膨胀
```

### 错误 2：忘了 extern template

```cpp
// header.h（被 100 个 .cpp 包含）
template<class T> class BigContainer { /* 1000 行 */ };
// 没有 extern template → 100 个 .cpp 各自实例化 → 编译慢 + 对象文件大
// 修正：extern template class BigContainer<int>;
```

### 错误 3：模板定义放在 .cpp

```cpp
// stack.h
template<class T> class Stack {
    void push(const T&);  // 声明
};
// stack.cpp
template<class T> void Stack<T>::push(const T& val) { /* 定义 */ }
// 其他 .cpp 包含 stack.h → 找不到定义 → 链接错误
// 修正：模板定义放在头文件
```

---

## 和 C 的区别

| 特性 | C void*/宏 | C++ 模板 |
|------|-----------|---------|
| 类型安全 | 无（void* 可任意 cast） | 有（编译期检查） |
| 代码量 | 一份（void*） | 每种类型一份（膨胀） |
| 性能 | 差（间接 + 无内联） | 好（直接 + 可内联） |
| 编译速度 | 快 | 慢（每种类型生成代码） |
| 二进制 | 小 | 大（代码膨胀） |

---

## HFT 关联

1. **模板代码膨胀**：过度模板化增大二进制 + I-cache 压力。HFT 对热路径模板适度，必要时用 `extern template` 显式实例化。
2. **extern template 集中编译**：大型项目用 `extern template` 集中实例化热路径模板（如 `Vector<Order>`），减少重复编译 + 代码膨胀。
3. **模板 vs void***：模板类型安全 + 可内联 + 编译期优化——比 C 的 void* 快。但代价是代码膨胀。HFT 权衡：热路径用模板（性能优先），非热路径用 void* 或多态（体积优先）。

---

## 代码自测

### Q1: 代码膨胀

```cpp
template<class T> T add(T a, T b) { return a + b; }
auto x = add(1, 2);
auto y = add(1.0, 2.0);
auto z = add(1L, 2L);
// 编译器生成了几份 add 的代码？
```

<details>
<summary>答案与复习指引</summary>

3 份：`add<int>`、`add<double>`、`add<long>`。模板为每种使用的类型生成独立代码。这就是代码膨胀——类型安全 + 可内联的代价是二进制增大。

**复习：** → [7.1 模板实例化](./01-template-instantiation.md)
</details>

### Q2: extern template

```cpp
// header.h
template<class T> class BigVec { /* 500 行 */ };
extern template class BigVec<int>;

// big_vec_int.cpp
template class BigVec<int>;

// main.cpp
#include "header.h"
BigVec<int> v;  // 会实例化吗？
```

<details>
<summary>答案与复习指引</summary>

不会实例化。`extern template class BigVec<int>` 告诉编译器"别在这里实例化，已经在别处实例化了"。`main.cpp` 中的 `BigVec<int> v;` 只声明不生成代码——链接时从 big_vec_int.cpp 的实例化中获取。好处：减少编译时间 + 避免重复代码。

**复习：** → [7.1 模板实例化](./01-template-instantiation.md)
</details>

### Q3: 模板定义位置

```cpp
// myvec.h
template<class T> class MyVec {
    void push(const T&);
};
// myvec.cpp
template<class T> void MyVec<T>::push(const T& val) { /* 定义 */ }
// user.cpp
#include "myvec.h"
MyVec<int> v;
v.push(42);  // 会链接成功吗？
```

<details>
<summary>答案与复习指引</summary>

链接失败。模板定义在 .cpp 中，user.cpp 看不到定义，无法实例化 `MyVec<int>::push`。修正：把模板定义放在头文件中（.h），或在 .cpp 中显式实例化 `template class MyVec<int>;`。

**复习：** → [7.1 模板实例化](./01-template-instantiation.md)
</details>

### Q4: 模板 vs void*

```cpp
// 方案 A：模板
template<class T> T max_val(T a, T b) { return a > b ? a : b; }

// 方案 B：void*
void* max_val(void* a, void* b, int (*cmp)(void*, void*));

// HFT 热路径选哪个？为什么？
```

<details>
<summary>答案与复习指引</summary>

方案 A（模板）。模板类型安全 + 可内联 + 编译期优化——`max_val(3, 4)` 可能内联为 `3 > 4 ? 3 : 4` → 编译期计算。void* 方案有函数指针间接调用 + 不可内联 + 无类型安全。代价是代码膨胀（每种类型一份），但 HFT 热路径优先性能。

**复习：** → [7.1 模板实例化](./01-template-instantiation.md)
</details>

---

## 参考与延伸

- 下一节：[7.2 异常处理实现](02-exception-impl.md)
- 回到：[第 7 章 高级对象模型](README.md)
