# Item 39：单次事件用 future + promise / condition_variable

> 第 7 章 · Item 39 · 上一节：[Item 38 句柄析构行为](item38-handle-destruction.md)

## 这节讲什么

经典模式：一个线程等另一个线程"完成一次初始化"。用 `promise<void>` 发信号 + `future.get()` 阻塞等待，比手写 condition_variable + flag 更不易错。

---

## 两种方式对比

### 方式 1：promise + future

```cpp
std::promise<void> p;
auto fut = p.get_future();
std::thread t([&]{
    initSomething();   // 完成初始化
    p.set_value();     // 发信号
});
fut.get();             // 阻塞等待初始化完成
t.join();
```

优势：无虚假唤醒、无锁泄漏、一次性。

### 方式 2：condition_variable + flag

```cpp
std::condition_variable cv;
std::mutex mtx;
bool ready = false;
// 线程 A: { lock_guard lk(mtx); ready = true; } cv.notify_one();
// 线程 B: { unique_lock lk(mtx); cv.wait(lk, [&]{ return ready; }); }
```

更繁琐，容易写错（忘记锁、虚假唤醒）。

### C++20：latch / barrier

```cpp
std::latch init_done(1);
// 线程 A: init_done.count_down();
// 线程 B: init_done.wait();
```

最简洁的标准化封装。

---

## 新手要点

- **新手用 promise+future**：比 condition_variable 简单且不易错。
- **C++20 有 latch**：如果编译器支持 C++20，`std::latch` 是最简洁的单次同步原语。

---

## HFT 关联

- **初始化等待**：主线程等网络线程完成交易所连接初始化后再开始下单。

---

## 自测题

1. `promise<void>` + `future.get()` 相比 condition_variable + flag 有什么优势？
2. condition_variable 的虚假唤醒是什么？promise+future 有这个问题吗？
3. C++20 的 `std::latch` 解决什么问题？

---

## 参考与延伸

- 下一节：[Item 40 atomic vs volatile](item40-atomic-vs-volatile.md)
- 回到：[第 7 章](README.md)
