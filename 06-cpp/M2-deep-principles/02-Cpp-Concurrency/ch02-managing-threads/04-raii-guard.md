# 2.4 RAII 守卫

> 第 2 章 · 上一节：[2.3 转移所有权](03-transferring-ownership.md) · 下一节：[2.5 硬件并发数](05-hardware-concurrency.md)

## 这节讲什么

用 RAII 包装 `std::thread`，保证所有路径（含异常）都安全收尾。

---

## RAII 守卫

```cpp
class joining_thread {
    std::thread t;
public:
    explicit joining_thread(std::thread&& th) : t(std::move(th)) {}
    ~joining_thread() { if (t.joinable()) t.join(); }
    joining_thread(const joining_thread&) = delete;
    joining_thread& operator=(const joining_thread&) = delete;
};
```

析构时自动 join——异常路径也安全。C++20 的 `std::jthread` 是标准化的版本（带协作式中断）。

---

## 新手要点

- **RAII 是 C++ 的核心范式**：资源获取即初始化——构造获取、析构释放。`thread` 的 RAII 守卫确保不 `terminate`。
- **`delete` 拷贝**：守卫不可拷贝（否则两个守卫管同一线程，双重 join）。

---

## HFT 关联

- **守护进程防崩**：HFT 守护进程里 `thread` 析构 terminate 会拉崩进程，用 RAII 守卫或 C++20 `jthread`。

---

## 自测题

1. RAII 守卫如何保证所有路径安全收尾？
2. 为什么守卫要 `delete` 拷贝构造？
3. C++20 的 `jthread` 和 RAII 守卫有什么关系？

---

## 参考与延伸

- 下一节：[2.5 硬件并发数](05-hardware-concurrency.md)
- 回到：[第 2 章](README.md)
