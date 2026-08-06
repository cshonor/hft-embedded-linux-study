# Item 25：对万能引用用 std::forward，对右值引用用 std::move

> 第 5 章 · Item 25 · 上一节：[Item 24 万能引用 vs 右值引用](item24-universal-vs-rvalue.md)

## 这节讲什么

混用 `move` 和 `forward` 会导致意外拷贝或意外移动。规则很简单：万能引用用 `forward`，右值引用用 `move`。

---

## 核心规则

```cpp
template<class T>
void set(T&& x) { target(std::forward<T>(x)); }  // 万能引用 → forward

void take(Widget&& w) { target(std::move(w)); }  // 右值引用 → move
```

**致命错误**：对万能引用用 `std::move`——若实参是左值，`move` 会无条件搬走它，调用方的左值被掏空，悬垂。

```cpp
template<class T>
void bad(T&& x) { target(std::move(x)); }  // 危险！左值实参被掏空
```

---

## 新手要点（和 C 的区别）

- **口诀**：万能引用配 `forward`，右值引用配 `move`。不确定是哪种引用？回到 Item 24 的判断标准。
- **`forward` 需要 `<T>`**：`std::forward<T>(x)` 必须显式传模板参数 `T`，编译器无法自动推导。

---

## HFT 关联

- **回调转发**：`template<class F> void onEvent(F&& cb) { storage.push_back(std::forward<F>(cb)); }` 保留 cb 的左右值性，避免不必要的拷贝。

---

## 自测题

1. 对万能引用用 `std::move` 为什么危险？
2. 什么时候用 `std::move`？什么时候用 `std::forward`？
3. `std::forward<T>(x)` 为什么必须显式传 `T`？

---

## 参考与延伸

- 下一节：[Item 26 避免万能引用重载](item26-avoid-overloading-universal.md)
- 回到：[第 5 章](README.md)
