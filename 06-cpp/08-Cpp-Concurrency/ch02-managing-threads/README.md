# 第 2 章 管理线程

**Managing Threads**

## 本章讲什么

`std::thread` 的完整生命周期管理——创建、`join`/`detach`、传参、转移所有权、RAII 防泄漏。这是 C++ 并发的最基本操作单元。

## 要点

### `std::thread` 生命周期

```cpp
std::thread t(func);   // 创建即开始运行
t.join();              // 等待完成
// 或 t.detach();       // 分离，后台运行
```
- `join`：阻塞调用线程直到 `t` 完成。
- `detach`：分离，`t` 在后台独立运行（失去句柄）。
- 析构时仍 `joinable`（未 join/detach）→ **`std::terminate`**。

### 传参

默认按**值拷贝**传参。要传引用用 `std::ref`；要传指针注意生命周期（悬垂）。

### 转移所有权

`std::thread` 不可拷贝，可移动：`std::thread t2 = std::move(t1);`。这让线程可存入容器（`vector<std::thread>`）。

### RAII 守卫

```cpp
class joining_thread {
    std::thread t;
public: ~joining_thread() { if(t.joinable()) t.join(); }
};
```
保证所有路径（含异常）都安全收尾。

### 硬件并发数

`std::thread::hardware_concurrency()` 返回硬件线程数——决定线程池大小的参考。

## HFT 关联

- **固定线程 + 绑核**：HFT 不在热路径创建/销毁线程（开销 + 抖动），而是启动时建固定线程池 + `pthread_setaffinity` 绑核，消除调度抖动。
- **RAII 守卫**：守护进程里 `thread` 析构 terminate 会拉崩进程，用 RAII 守卫或显式 join。

## 自测题

1. `std::thread` 析构时仍 joinable 会怎样？如何用 RAII 规避？
2. 传引用给线程函数要怎么做？传指针有什么风险？
3. HFT 为什么不在热路径创建线程？怎么固定线程？
