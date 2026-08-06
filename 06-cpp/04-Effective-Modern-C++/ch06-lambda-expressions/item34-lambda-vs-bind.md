# Item 34：优先 lambda 而非 std::bind

> 第 6 章 · Item 34 · 上一节：[Item 33 泛型 lambda](item33-generic-lambda.md)

## 这节讲什么

C++14 起几乎所有 `std::bind` 场景都该用 lambda 替代——lambda 可内联、参数清晰、支持 move-only 类型。

---

## bind 的缺陷

1. **无法内联**：`bind` 是函数调用，lambda 可内联
2. **参数占位符晦涩**：`_1`、`_2` 不直观
3. **重载/模板函数**：传给 bind 需要显式类型转换，lambda 不需要
4. **move-only 类型**：bind 的值传递语义对 `unique_ptr` 不友好

---

## 新手要点

- **一律用 lambda**：新代码别用 `bind`，除非极少数"运行时组合调用链"的场景。
- **lambda 更易读**：`[](const auto& a, const auto& b) { return a.x < b.x; }` 比 `bind(less<>{}, bind(&Point::x, _1), bind(&Point::x, _2))` 清晰得多。

---

## HFT 关联

- **lambda 内联**：STL 算法传 lambda 比 `bind`/函数指针更易内联——回测里对 tick 数组批量处理时，内联 lambda 性能显著优于函数指针。

---

## 自测题

1. `std::bind` 相比 lambda 有哪些缺陷？
2. 为什么 C++14 起几乎都该用 lambda？
3. lambda 可内联为什么对 HFT 重要？

---

## 参考与延伸

- 下一章：[第 7 章 并发 API](../ch07-concurrency-api/README.md)
- 回到：[第 6 章](README.md)
