# Item 40–42：函数适配器（ptr_fun / mem_fun / mem_fun_ref）

> 第 6 章 仿函数与函数 · Items 40–42 · 上一节：[Item 39 谓词可配对](item39-predicate-adaptable.md) · 下一节：[Item 43 仿函数优于函数指针](item43-functor-vs-function-pointer.md)

## 为什么要学这个（先建立直觉）

在 C 里，调用结构体的方法只能手写循环——没有"把成员函数传给算法"的机制。

```c
/* C: 手写循环调用结构体"方法" */
struct Widget { int value; };
void widget_do_work(struct Widget* w) { w->value *= 2; }

for (int i = 0; i < n; i++) {
    widget_do_work(&widgets[i]);  // 手动循环
}
```

```cpp
// C++03: 用 mem_fun_ref 把成员函数包装成仿函数
std::for_each(widgets.begin(), widgets.end(),
    std::mem_fun_ref(&Widget::doWork));

// C++11+: lambda 直接写，适配器完全不需要
std::for_each(widgets.begin(), widgets.end(),
    [](Widget& w) { w.doWork(); });
```

**直觉**：C++03 的函数适配器把"普通函数指针"或"成员函数指针"包装成仿函数，让 STL 算法能用。C++11 后 lambda 基本完全替代了它们。

## 这节讲什么

三个适配器（全部在 `<functional>` 中，C++11 起废弃，C++17 移除）：

### Item 40：`ptr_fun` — 包装普通函数指针

```cpp
bool is_even(int x) { return x % 2 == 0; }

// 不用适配器也能传函数指针
std::find_if(v.begin(), v.end(), is_even);

// 用 ptr_fun 包装（让函数指针"可配对"）
std::find_if(v.begin(), v.end(), std::not1(std::ptr_fun(is_even)));
// ptr_fun 提供了 argument_type，使 not1 可用
```

### Item 41：`mem_fun` — 包装成员函数指针（对象指针）

```cpp
std::vector<Widget*> widget_ptrs;
std::for_each(widget_ptrs.begin(), widget_ptrs.end(),
    std::mem_fun(&Widget::doWork));  // 对指针调用 doWork()
```

### Item 42：`mem_fun_ref` — 包装成员函数指针（对象本身）

```cpp
std::vector<Widget> widgets;
std::for_each(widgets.begin(), widgets.end(),
    std::mem_fun_ref(&Widget::doWork));  // 对对象调用 doWork()
```

### 三者区别

| 适配器 | 包装什么 | 调用方式 | 等价 lambda |
|--------|---------|----------|-------------|
| `ptr_fun(f)` | 普通函数 `f` | `f(x)` | `[&](auto& x) { return f(x); }` |
| `mem_fun(&C::m)` | 成员函数（指针容器） | `ptr->m()` | `[&](auto* p) { p->m(); }` |
| `mem_fun_ref(&C::m)` | 成员函数（对象容器） | `obj.m()` | `[&](auto& o) { o.m(); }` |

### 为什么 C++11 后不需要了

```cpp
// C++03: 三种适配器
std::find_if(v.begin(), v.end(), std::not1(std::ptr_fun(is_even)));
std::for_each(ptrs.begin(), ptrs.end(), std::mem_fun(&Widget::doWork));
std::for_each(objs.begin(), objs.end(), std::mem_fun_ref(&Widget::doWork));

// C++11: lambda 统一替代
std::find_if(v.begin(), v.end(), [](int x) { return x % 2 != 0; });
std::for_each(ptrs.begin(), ptrs.end(), [](Widget* p) { p->doWork(); });
std::for_each(objs.begin(), objs.end(), [](Widget& o) { o.doWork(); });
```

lambda 更清晰、更灵活、可内联，完全没有理由在新代码中使用这些废弃适配器。

### C++11 替代：std::function + std::bind

```cpp
#include <functional>

// std::bind 部分替代 mem_fun
auto fn = std::bind(&Widget::doWork, std::placeholders::_1);
std::for_each(objs.begin(), objs.end(), fn);

// 但 lambda 通常比 bind 更好
std::for_each(objs.begin(), objs.end(), [](Widget& w) { w.doWork(); });
```

## 常见错误（新手踩坑）

### 错误 1：mem_fun vs mem_fun_ref 混用

```cpp
std::vector<Widget> objs;
std::for_each(objs.begin(), objs.end(),
    std::mem_fun(&Widget::doWork));  // 错！mem_fun 是给指针用的
```

**修复**：对象容器用 `mem_fun_ref`，指针容器用 `mem_fun`。或者直接用 lambda。

### 错误 2：在 C++17 代码中使用已删除的适配器

```cpp
// C++17: ptr_fun/mem_fun/mem_fun_ref 已删除
std::for_each(v.begin(), v.end(), std::ptr_fun(do_something));  // 编译错误
```

**修复**：用 lambda 替代。

### 错误 3：过度使用 std::bind

```cpp
// 可行但难以阅读
auto fn = std::bind(std::greater<int>(), std::placeholders::_1, 5);
std::find_if(v.begin(), v.end(), fn);

// lambda 更清晰
auto it = std::find_if(v.begin(), v.end(), [](int x) { return x > 5; });
```

## 新手要点（和 C 的区别）

| 方面 | C | C++03 | C++11+ |
|------|---|-------|--------|
| 调用函数指针 | 直接传 `qsort` | `ptr_fun` 配对 | lambda / 直接传 |
| 调用成员函数 | 手写循环 | `mem_fun`/`mem_fun_ref` | lambda |
| 部分应用 | 无 | `bind1st`/`bind2nd` | `std::bind` 或 lambda |
| 可读性 | — | 差（嵌套适配器） | 好（lambda 直观） |

## HFT 关联

- **lambda 全面替代适配器**：HFT 代码中用 `[&](const Tick& t){ strategy.on_tick(t); }` 清晰且可内联
- **std::function 的开销**：`std::function` 有类型擦除开销（堆分配 + 虚调用），热路径避免使用，用模板参数或 lambda 替代
- **bind 的性能陷阱**：`std::bind` 可能产生额外拷贝，lambda 按值/引用捕获更可控

## 代码自测

### Q1: mem_fun vs mem_fun_ref

```cpp
std::vector<Widget> objs;
std::vector<Widget*> ptrs;
// 分别用 C++03 适配器调用 doWork
```
> objs 和 ptrs 分别应该用哪个适配器？

<details>
<summary>答案</summary>

- `objs`（对象容器）→ `std::mem_fun_ref(&Widget::doWork)`
- `ptrs`（指针容器）→ `std::mem_fun(&Widget::doWork)`

C++11+ 统一用 lambda：
```cpp
std::for_each(objs.begin(), objs.end(), [](Widget& w) { w.doWork(); });
std::for_each(ptrs.begin(), ptrs.end(), [](Widget* p) { p->doWork(); });
```
</details>

### Q2: ptr_fun 的唯一用途

```cpp
bool is_positive(int x) { return x > 0; }
// A:
std::find_if(v.begin(), v.end(), is_positive);
// B:
std::find_if(v.begin(), v.end(), std::not1(std::ptr_fun(is_positive)));
```
> 什么时候必须用 `ptr_fun`？

<details>
<summary>答案</summary>

只有需要 `not1`/`not2` **配对**时才需要 `ptr_fun`——它给函数指针添加 `argument_type` typedef。

单纯传函数指针（A 方式）不需要 `ptr_fun`。

C++11+ 完全不需要：直接用 lambda `[](int x) { return x <= 0; }` 替代 `not1(ptr_fun(...))`。
</details>

### Q3: lambda 替代

```cpp
// C++03: 写出等价的 lambda
std::for_each(widgets.begin(), widgets.end(),
    std::mem_fun_ref(&Widget::process));
```

<details>
<summary>答案</summary>

```cpp
std::for_each(widgets.begin(), widgets.end(),
    [](Widget& w) { w.process(); });
```

如果 `process` 是 const 成员函数且不修改对象：
```cpp
std::for_each(widgets.begin(), widgets.end(),
    [](const Widget& w) { w.process(); });
```
</details>

### Q4: std::function vs lambda

```cpp
// A: std::function
std::function<bool(int)> pred = [threshold=100](int x) { return x > threshold; };

// B: 直接 lambda（auto 推导）
auto pred2 = [threshold=100](int x) { return x > threshold; };

std::find_if(v.begin(), v.end(), pred);
std::find_if(v.begin(), v.end(), pred2);
```
> A 和 B 在性能上有什么区别？

<details>
<summary>答案</summary>

**B 更快**。`auto pred2` 推导为闭包类型（编译器生成的唯一类），`find_if` 模板实例化时类型已知，`operator()` 可内联。

**A 用 std::function**：类型擦除 → `operator()` 通过虚调用或函数指针间接调用 → **无法内联**。且 `std::function` 可能堆分配（如果闭包大于 SBO 缓冲区）。

**HFT**：热路径用 `auto` + lambda，避免 `std::function`。`std::function` 用于需要"存储可调用对象"的场景（如回调注册表），不在热路径上。
</details>

## 参考与延伸

- 上一节：[Item 39 谓词可配对](item39-predicate-adaptable.md)
- 下一节：[Item 43 仿函数优于函数指针](item43-functor-vs-function-pointer.md)
- [Effective Modern C++ Item 34：lambda 优于 bind](../../../M1-modern-cpp/01-Effective-Modern-C++/ch06-lambda-expressions/README.md)
