# 第 8 章 微调

**Tweaks** — Items 41–42

## 本章讲什么

最后两条 item 是对前述机制的精细微调：可拷贝参数的传参方式选择、`emplace` 系列替代 `insert` 系列。它们看似细枝末节，却在泛型库设计和高性能容器操作中带来真实收益。

---

## 各 Item 要点

### Item 41：可拷贝参数考虑按值传递

对于"会被拷贝的可拷贝形参"，传统做法是传 `const T&` 再在函数内拷贝。但若函数**总是**要拷贝这个参数，且参数可能以左值或右值传入，按值传递能简化代码：

```cpp
// 传统：两个重载
void add(const T& x) { v.push_back(x); }      // 左值：一次拷贝
void add(T&& x)      { v.push_back(std::move(x)); }  // 右值：一次移动

// 按值：一个函数搞定
void add(T x) { v.push_back(std::move(x)); }  // 左值：拷贝构造形参 + 移动；右值：移动构造 + 移动
```

**代价**：按值传递对左值多了一次移动（拷贝构造形参 + 移动进容器），传统 `const&` 重载只拷贝一次。所以只有当**移动廉价**（如 `string`、`vector`、智能指针）时才值得；移动昂贵的类型（`array<T,N>`、`BigStruct`）仍用重载。

**适用条件**：①函数总是拷贝形参；②移动廉价；③拷贝构造与移动构造代价相近。不满足任一条，用重载或万能引用 + `forward`。

### Item 42：优先 `emplace` 而非 `insert`

`emplace_back` / `emplace` 在容器内**直接构造**元素，省去临时对象 + 移动/拷贝：

```cpp
v.push_back(Widget(42));     // 构造临时 Widget → 移动进 v
v.emplace_back(42);          // 直接在 v 的内存里构造 Widget
```

**emplace 的优势**：无临时对象、无移动、可传任意构造参数（含 explicit 构造）。

**emplace 的限制**：
1. **依赖 `value_type` 可直接构造**：若 `value_type` 与传入参数间需要隐式转换且转换路径不止一条，`emplace` 可能选错构造函数（与 `push_back` 的明确转换行为不同）。
2. **资源管理顺序**：`push_back` 先构造临时对象再插入，若插入失败（如扩容抛异常）临时对象已析构，无泄漏；`emplace` 直接在容器内存构造，若后续扩容/拷贝抛异常，需依赖容器的强异常保证（`vector` 的 `emplace_back` 在 `noexcept` 移动时才强保证）。
3. **与 `unique_ptr` / 共享状态**：`emplace_back(new Widget)` 若 `vector` 扩容抛异常，裸指针泄漏——应 `emplace_back(make_unique<Widget>())` 或 `push_back(make_unique<Widget>())`。

**经验法则**：能用 `emplace` 就用，除非需要明确控制转换路径或异常安全特别敏感。`push_back(make_unique<T>())` 这种"先构造智能指针再插入"的模式仍是最安全的。

---

## HFT 关联

- **`emplace_back` 热路径收益**：行情队列 `vector<Tick>` 批量入队用 `emplace_back` 省去临时 `Tick` 的构造 + 移动——当 `Tick` 含多个字段（含 `string` symbol）时，省下的拷贝在每秒百万 tick 的吞吐下是可观的微秒级收益。
- **异常安全 vs 性能**：HFT 容器若在 `emplace` 路径抛异常（扩容 `bad_alloc`），裸指针资源会泄漏。规则——`emplace_back` 传智能指针/值类型，不传裸 `new` 结果。
- **按值传参与移动**：策略对象的字段（`string` symbol、`vector<Param>`）移动廉价，配置注册函数用按值传参 + `std::move` 进成员，代码简洁且性能可接受。

---

## 自测题

1. 按值传递可拷贝形参相比 `const T&` + 拷贝，对左值实参多出了什么代价？什么条件下按值传递才划算？
2. `emplace_back(42)` 相比 `push_back(Widget(42))` 省掉了什么？什么场景下 `emplace` 反而不如 `push_back`？
3. `v.emplace_back(new Widget)` 在扩容抛异常时会发生什么？正确的写法是什么？
4. 为什么 `emplace` 对"需要明确转换路径"的场景有风险？
5. `push_back(make_unique<T>())` 为什么是 `unique_ptr` 容器插入的最安全写法？
