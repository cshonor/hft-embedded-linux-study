# 2.2 传参

> 第 2 章 · 上一节：[2.1 thread 生命周期](01-thread-lifecycle.md) · 下一节：[2.3 转移所有权](03-transferring-ownership.md)

## 这节讲什么

`std::thread` 默认按值拷贝传参。要传引用用 `std::ref`；传指针要注意生命周期。

---

## 传参方式

```cpp
void f(int x, std::string& s);

std::string s = "hello";
std::thread t(f, 42, std::ref(s));  // 传引用用 std::ref
```

- **默认按值拷贝**：参数被拷贝到线程内部存储
- **传引用**：用 `std::ref(s)` / `std::cref(s)`
- **传指针**：注意生命周期——指针指向的局部变量可能在线程运行时已销毁（悬垂）

---

## 新手要点

- **别传局部变量的引用/指针**：线程函数可能在局部变量销毁后才运行——悬垂引用 UB。
- **传移动语义**：`std::thread t(f, std::move(obj));` 把 `unique_ptr` 等移动进线程。

---

## 自测题

1. `std::thread` 默认如何传参？要传引用怎么做？
2. 传指针给线程有什么风险？
3. 如何把 `unique_ptr` 传给线程函数？

---

## 参考与延伸

- 下一节：[2.3 转移所有权](03-transferring-ownership.md)
- 回到：[第 2 章](README.md)
