# 2.3 转移所有权

> 第 2 章 · 上一节：[2.2 传参](02-passing-args.md) · 下一节：[2.4 RAII 守卫](04-raii-guard.md)

## 这节讲什么

`std::thread` 不可拷贝，可移动——这让线程可存入容器（`vector<std::thread>`）。

---

## 移动语义

```cpp
std::thread t1(func);
std::thread t2 = std::move(t1);  // t1 失去句柄，t2 接管线程
// t1 此后不再 joinable

std::vector<std::thread> pool;
pool.push_back(std::thread(func1));  // 移动进容器
pool.push_back(std::move(t2));
```

`std::thread` 不可拷贝（拷贝会导致两个句柄管同一线程），只能移动。

---

## 新手要点

- **移动后原 thread 不再 joinable**：`std::move(t1)` 后 `t1` 是空的，析构不会 `terminate`。
- **存入容器**：`vector<std::thread>` 是线程池的基础——线程对象存容器，统一管理 join。

---

## 自测题

1. 为什么 `std::thread` 不可拷贝但可移动？
2. `std::move(t1)` 后 `t1` 的状态是什么？
3. 如何把线程存入 `vector`？

---

## 参考与延伸

- 下一节：[2.4 RAII 守卫](04-raii-guard.md)
- 回到：[第 2 章](README.md)
