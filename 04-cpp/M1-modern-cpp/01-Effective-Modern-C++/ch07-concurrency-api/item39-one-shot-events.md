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

<details>
<summary>参考答案</summary>

1. ①**代码极简**：`promise.set_value()` + `future.get()` 两行就完成"一次通知"，不需要额外的 flag、mutex、条件判断。②**不存在虚假唤醒与丢失唤醒**：`get()` 只关心"共享状态是否就绪"，等待者在 `set_value()` 之前调用也不会错过事件（状态是持久的），不需要 `while (!flag) cv.wait(...)` 这种循环和谓词。③**不需要互斥量**：condition_variable 必须配 `std::mutex` 保护共享 flag，promise/future 的共享状态本身是线程安全的，少一把锁、少一次加锁开销。④**能传递结果与异常**：`future` 可携带返回值，或用 `set_exception()` 把异常传播到等待方；cv + flag 只能表示"发生了"。⑤**一次性语义明确**：promise 只能 `set_value` 一次，正好匹配"初始化完成"这类单次事件。

2. 虚假唤醒（spurious wakeup）指等待在 condition_variable 上的线程**可能在没有收到任何 `notify` 的情况下被唤醒**——标准允许实现这么做（为的是在某些平台上效率更高），因此正确代码必须把 `wait` 放在谓词循环里：
```cpp
std::unique_lock<std::mutex> lk(m);
cv.wait(lk, []{ return ready; });   // 必须带谓词
```
只用 flag + 一次 `wait` 的写法在虚假唤醒时会提前继续，误以为事件已发生。promise + future **没有**这个问题：`future::get()` 只在共享状态真正就绪时返回，中间没有任何"无缘无故醒来"的可能，语义由标准保证。

3. `std::latch`（C++20）是一个**一次性的、向下计数的同步点**：用初始计数值 N 构造，工作线程完成任务后 `count_down()`，等待方 `wait()` 直到计数归零才被唤醒。它解决的是"等待 N 个事件/线程全部完成"这一常见模式——如等 N 个行情连接/初始化线程就绪后再开始下单——比手写"计数器 + mutex + condition_variable"简洁得多，且不要求参与方持有锁、也不会虚假唤醒。与 `std::barrier` 的区别是：`latch` 是一次性的（计数到 0 后不能重置），`barrier` 可重复使用于多轮同步。单次事件（N=1）用 `promise/future` 或 `latch(1)` 都可以，多事件汇聚用 `latch` 更自然。

</details>

---

## 参考与延伸

- 下一节：[Item 40 atomic vs volatile](item40-atomic-vs-volatile.md)
- 回到：[第 7 章](README.md)
