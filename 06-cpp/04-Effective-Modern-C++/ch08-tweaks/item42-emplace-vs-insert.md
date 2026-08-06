# Item 42：优先 emplace 而非 insert

> 第 8 章 微调 · Item 42 · 上一节：[Item 41 按值传递](item41-pass-by-value.md)

## 这节讲什么

`emplace_back`/`emplace` 在容器内**直接构造**元素，省去临时对象 + 移动/拷贝。但要注意异常安全和资源管理。

---

## 核心对比

```cpp
v.push_back(Widget(42));     // 构造临时 Widget → 移动进 v
v.emplace_back(42);          // 直接在 v 的内存里构造 Widget
```

`emplace` 的优势：无临时对象、无移动、可传任意构造参数。

### emplace 的限制

1. **依赖 value_type 可直接构造**：若转换路径不止一条，`emplace` 可能选错构造函数。
2. **异常安全**：`emplace_back(new Widget)` 若 `vector` 扩容抛异常，裸指针泄漏——应 `emplace_back(make_unique<Widget>())`。
3. **与 `push_back` 的明确转换**不同：`emplace` 更灵活但也更容易选错构造函数。

**经验法则**：能用 `emplace` 就用，除非需要明确控制转换路径。`push_back(make_unique<T>())` 是 `unique_ptr` 容器插入的最安全写法。

---

## 新手要点

- **`emplace_back` 省临时对象**：`push_back(Widget(42))` 先构造临时再移动，`emplace_back(42)` 直接在容器内存构造——省了临时对象的构造+移动。
- **别传裸 new**：`emplace_back(new Widget)` 有异常泄漏风险，传 `make_unique` 或 `make_shared`。

---

## HFT 关联

- **行情队列**：`vector<Tick>` 批量入队用 `emplace_back` 省去临时 `Tick` 的构造 + 移动——含 `string` symbol 字段时微秒级收益可观。
- **异常安全规则**：`emplace_back` 传智能指针/值类型，不传裸 `new` 结果。

---

## 自测题

1. `emplace_back(42)` 相比 `push_back(Widget(42))` 省掉了什么？
2. `v.emplace_back(new Widget)` 在扩容抛异常时会发生什么？正确写法是什么？
3. 什么场景下 `emplace` 反而不如 `push_back`？
4. 为什么 `push_back(make_unique<T>())` 是 `unique_ptr` 容器插入的最安全写法？

---

## 参考与延伸

- 下一章：[第 9 章（无）—— 本书结束](../README.md)
- 回到：[第 8 章 微调](README.md)
