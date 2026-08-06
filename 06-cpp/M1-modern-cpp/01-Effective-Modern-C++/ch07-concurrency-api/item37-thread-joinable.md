# Item 37：让 std::thread 在所有路径都不可联结（joinable）

> 第 7 章 · Item 37 · 上一节：[Item 36 启动策略](item36-launch-policy.md)

## 这节讲什么

`std::thread` 析构时若仍 `joinable`（既未 `join` 也未 `detach`）→ **`std::terminate`**。用 RAII 保证所有路径安全。

---

## 核心问题

```cpp
void f() {
    std::thread t(work);
    // ... 如果这里抛异常或 return ...
}  // t 析构时仍 joinable → std::terminate！
```

### RAII 守卫

```cpp
class ThreadGuard {
    std::thread t;
public:
    explicit ThreadGuard(std::thread&& th) : t(std::move(th)) {}
    ~ThreadGuard() { if (t.joinable()) t.join(); }
    ThreadGuard(const ThreadGuard&) = delete;
    ThreadGuard& operator=(const ThreadGuard&) = delete;
};
```

---

## 新手要点（和 C 的区别）

- **C 用 pthread**：C 程序员习惯 `pthread_create` + 手动 `pthread_join`。C++ 的 `std::thread` 析构时如果仍 joinable 会直接 `terminate` 整个进程——这是比 pthread 更严格的安全检查。
- **规则**：每创建一个 `std::thread`，确保在所有路径（含异常路径）都 `join` 或 `detach`。最安全的方式是 RAII 守卫。

---

## HFT 关联

- **守护进程崩溃**：HFT 守护进程里 `std::thread` 析构时若仍 joinable 会 `terminate` 拉崩整个进程——用 RAII 守卫或显式 `join`/`detach`。

---

## 自测题

1. `std::thread` 析构时仍 joinable 会发生什么？
2. 如何用 RAII 规避这个问题？
3. 为什么 `ThreadGuard` 要 `delete` 拷贝构造？

---

## 参考与延伸

- 下一节：[Item 38 句柄析构行为](item38-handle-destruction.md)
- 回到：[第 7 章](README.md)
