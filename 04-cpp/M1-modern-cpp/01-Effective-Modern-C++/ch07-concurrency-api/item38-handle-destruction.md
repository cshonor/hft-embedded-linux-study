# Item 38：理解线程句柄的析构与联结行为

> 第 7 章 · Item 38 · 上一节：[Item 37 thread joinable](item37-thread-joinable.md)

## 为什么要学这个（先建立直觉）

C 程序员管理线程句柄很简单——`pthread_t` 就是个整数 ID，析构（离开作用域）不做任何事：

```c
pthread_t tid;
pthread_create(&tid, NULL, worker, NULL);
// tid 离开作用域——什么都不会发生
// 线程继续在后台跑，资源在进程结束时回收
```

C++ 的线程句柄有不同的析构行为，混用会出问题：

- `std::thread` 析构时如果 joinable → **terminate**（进程崩溃）
- `std::future` 析构时如果任务未完成 → **阻塞等待**（和 `get()` 类似）
- `std::shared_future` 析构 → **不阻塞**（多个共享）

```cpp
{
    std::future<int> fut = std::async(std::launch::async, []{ long_task(); });
}  // fut 析构 → 阻塞等 long_task 完成！
// 以为是异步，实际在析构处同步阻塞
```

---

## 这节讲什么

不同线程句柄的析构行为不同——`thread` 析构会 `terminate`，`future` 析构会阻塞等待。

---

## 析构行为对比

| 句柄类型 | 析构行为 | 类比 |
|----------|----------|------|
| `std::thread` | joinable 则 `terminate` | C：忘了 join 只是泄漏，C++ 直接崩 |
| `std::future` | 阻塞等待（共享状态未就绪时） | C：无类比物 |
| `std::shared_future` | 不阻塞（多个 shared_future 共享状态） | C：无类比物 |

### future 析构阻塞

```cpp
// future 析构会阻塞——反直觉
{
    auto fut = std::async(std::launch::async, []{
        std::this_thread::sleep_for(std::chrono::seconds(5));
        return 42;
    });
    // fut 在这里析构 → 阻塞等待 5 秒！
}  // ← 程序在这里卡 5 秒

// 对比：如果不需要结果，用 shared_future 不会阻塞
{
    auto fut = std::async(std::launch::async, long_task).share();
    // shared_future 析构不阻塞
}  // 不卡
```

`future` 析构会阻塞——反直觉但有用：让你"忘了 `get` 也能拿到结果"。但也可能让程序在意外处卡住。

### 为什么 future 析构要阻塞？

```cpp
// 场景：异步任务写了共享变量，future 析构保证任务完成
int result;
auto fut = std::async(std::launch::async, [&]{ result = compute(); });
// fut 析构 → 等待 compute 完成 → result 已写入
// 如果 future 析构不等待 → result 可能还没写入就被读取
```

---

## 常见错误（新手踩坑）

**错误 1：在热路径析构 future 导致意外阻塞**
```cpp
void on_tick(const Tick& t) {
    auto fut = std::async(std::launch::async, check_risk, t);
    // 处理 tick...
    // fut 在函数末尾析构 → 阻塞等 check_risk 完成 → 热路径卡住！
}
```
**修正：** 把 `future` 存到后台线程，或用 `shared_future`。

**错误 2：以为 future 和 thread 析构行为相同**
```cpp
// thread 析构 = terminate（如果 joinable）
// future 析构 = 阻塞等待
// 完全不同！
```
**修正：** 记住口诀——`thread` = 炸，`future` = 等。

**错误 3：临时 future 析构立即阻塞**
```cpp
std::async(std::launch::async, long_task);  // 返回临时 future
// 临时 future 立即析构 → 立即阻塞等 long_task 完成 → 变成同步！
```
**修正：** 存到变量 `auto fut = std::async(...);` 延长生命周期。

---

## 新手要点（和 C 的区别）

| 维度 | C 怎么做 | C++ 怎么做 | 为什么 |
|------|---------|-----------|--------|
| 线程句柄 | `pthread_t`（整数 ID） | `thread`/`future`/`shared_future` | C++ 有 RAII |
| 析构行为 | 不做任何事 | 各有不同 | C++ 对象语义 |
| 异常路径 | 手动管理 | 析构自动处理 | RAII |

**一句话总结：** C 程序员记住——`thread` 析构 = 炸（terminate），`future` 析构 = 等（阻塞）。别在热路径析构 future。

---

## HFT 关联

- **异步风控**：异步风控任务用 `future`，但确保不在热路径析构 future——把 future 存到后台线程或用 `shared_future`。
- **热路径阻塞**：`on_tick` 中 `async` 返回的 future 在函数末尾析构会阻塞——把 future 移到成员变量或队列。
- **临时 future 陷阱**：`std::async(...)` 不存返回值 → 临时 future 立即析构 → 同步阻塞。

---

## 自测题

1. `std::thread` 和 `std::future` 的析构行为有何不同？
2. 为什么 `future` 析构会阻塞？这有什么用？
3. `shared_future` 析构为什么不阻塞？
4. 下面代码有什么问题？
```cpp
void on_tick(const Tick& t) {
    std::async(std::launch::async, check_risk, t);
    process(t);
}
```

---

## 参考与延伸

- 下一节：[Item 39 单次事件](item39-one-shot-events.md)
- 回到：[第 7 章](README.md)
