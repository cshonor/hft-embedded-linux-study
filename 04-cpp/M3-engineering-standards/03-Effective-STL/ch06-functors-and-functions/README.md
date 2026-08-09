# 第 6 章 仿函数、仿函数类、函数等

**Functors, Functor Classes, Functions, etc.** — Items 38–43

## 本章讲什么

仿函数（functor / function object）是重载 `operator()` 的类对象，可像函数一样调用。它是 STL 算法定制行为的手段——排序比较、累加操作、过滤谓词都用仿函数。C++11 后 lambda 在多数场景替代了手写仿函数，但仿函数的"有状态 + 可配对"特性仍有价值。本章讲仿函数设计规范与函数适配器。

---

## 各 Item 要点

### Item 38：仿函数按值传递

STL 算法按**值**拷贝仿函数（`for_each`/`transform` 等）。因此仿函数应轻量（拷贝廉价），且 `operator()` 应为 `const`（因为算法拷贝的是 const 副本）。有状态仿函数的状态会随拷贝分散——想让外部看到最终状态，用 `for_each` 的返回值（返回最后一次的仿函数副本）。

### Item 39：使仿函数可配对（predicate）

谓词仿函数应返回 `bool` 且 `operator()` 是 `const`，这样才能被 `not1`/`not2` 适配器配对。C++11 后 lambda 通常直接写否定逻辑，配对需求减少。

### Item 40–42：函数适配器（`ptr_fun`/`mem_fun`/`mem_fun_ref`）

这些适配器把普通函数指针 / 成员函数包装成仿函数，供算法使用：

```cpp
std::for_each(objs.begin(), objs.end(), std::mem_fun_ref(&Widget::doWork));
```

C++11 起 `std::function` + lambda 几乎完全替代了这些适配器。新代码用 lambda 更清晰。

### Item 43：优先传函数对象而非函数指针

函数指针阻止内联（编译器难跨间接调用优化）；仿函数 / lambda 类型唯一，编译器可内联 `operator()`。HFT 热路径用仿函数 / lambda 而非函数指针，换取内联与 SIMD 优化机会。

---

## HFT 关联

- **lambda 替代适配器**：HFT 代码里 `[&](const Tick& t){ ... }` 比 `mem_fun_ref` 清晰百倍，且可内联。新代码全面用 lambda。
- **有状态仿函数做累积**：回测里用有状态仿函数累积 PnL，`for_each` 返回最终副本取结果——但注意值拷贝语义，大状态用引用捕获或外部变量。
- **函数指针阻碍内联**：热路径排序比较器用 lambda 而非函数指针，让 `sort` 能内联比较逻辑，性能差距可达数倍。

---

## 自测题

1. STL 算法按值还是按引用传递仿函数？这对仿函数设计有什么约束？
2. 为什么仿函数的 `operator()` 应为 `const`？
3. C++11 后哪些函数适配器基本被 lambda 替代？
4. 函数指针相比仿函数/lambda 在性能上有什么劣势？为什么？
5. `for_each` 的返回值有什么用？有状态仿函数如何取最终状态？
