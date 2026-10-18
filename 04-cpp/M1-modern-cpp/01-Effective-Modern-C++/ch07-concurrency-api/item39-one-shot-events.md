# Item 39：单次事件用 future + promise / condition_variable

> 第 7 章 · Item 39 · 上一节：[Item 38 句柄析构行为](item38-handle-destruction.md)

## 为什么要学这个（先建立直觉）

并发编程里最高频的协作模式之一是："线程 B 必须等线程 A 完成**一次**某件事"——等连接建好、等配置加载完、等行情首包到达。教科书的标准答案是 `condition_variable` + flag + mutex 三件套，但这三件套有两颗经典地雷（虚假唤醒、丢失唤醒），每颗都足以让新手写下"九成九能跑"的代码。

`promise` + `future` 把同一个模式压缩成两行，而且**从机制上免疫**那两颗地雷。Meyers 的立场很明确：单次事件，默认用 promise/future；condition_variable 留给真正需要它的重复事件。

---

## 这节讲什么

1. promise/future 单次通信（实测可跑），以及它为什么免疫虚假唤醒与丢失唤醒；
2. condition_variable + flag 的正解写法与两颗地雷的原理；
3. C++20 `std::latch`：N 个事件的汇聚版（实测）；
4. promise/future 的隐藏能力：异常穿越线程边界。

---

## 方式一：promise + future（首选，实测）

```cpp
#include <future>
#include <thread>

std::promise<int> p;
auto fut = p.get_future();

std::thread t([&]{
    initSomething();        // 完成初始化
    p.set_value(42);        // 发信号，还能捎带结果
});

int result = fut.get();     // 阻塞，直到 set_value 发生
t.join();
```

clang 23.1.0 实测运行：主线程阻塞约 50ms 后拿到 `42`。

### 为什么它免疫两颗地雷

**丢失唤醒**：cv 版本里如果 A 在 B 开始 `wait` **之前**就 `notify`，这次通知没人接收就丢了——B 睡到地老天荒。promise 的共享状态是**持久**的：`set_value` 先发生，`fut.get()` 后调用照样立刻返回结果——事件发生过就记录在案，不存在"错过"。

**虚假唤醒**：标准允许 `condition_variable::wait` 在没人 `notify` 的情况下莫名返回（某些平台上的实现策略），所以正确写法必须带谓词循环（见下）。`future::get()` 只在共享状态真正就绪时返回——没有"醒来检查一下"这个中间态，标准直接保证了语义。

此外它还**少一把锁**：cv 方案必须用 mutex 保护 flag，promise 的共享状态自身线程安全。外加两个赠品：能携带结果（`promise<int>`）、能传播异常（下文）。

## 方式二：condition_variable + flag（必须会读）

```cpp
std::condition_variable cv;
std::mutex mtx;
bool ready = false;

// 线程 A（通知方）：
{
    std::lock_guard lk(mtx);
    ready = true;
}
cv.notify_one();

// 线程 B（等待方）：
{
    std::unique_lock lk(mtx);
    cv.wait(lk, []{ return ready; });   // ★ 谓词形式是底线，不是优化
}
```

两颗地雷的机理：

- **虚假唤醒** → 必须用带谓词的 `wait(lk, pred)`（等价于 `while (!pred()) wait(lk);`）。裸 `wait(lk)` + 事后不检查 flag，虚假唤醒时就带着 `ready == false` 往下跑。
- **丢失唤醒** → `ready = true` 必须在锁内写、且等待方进入 `wait` 前也要在锁内检查谓词（谓词形式替你做了）。notify 在 wait 之前到达时，靠"flag 已经置位"这个事实兜底，而不是靠 notify 本身。

cv + flag 真正不可替代的场景：**重复事件**（每来一个 tick 通知一次）、**多等待方广播**（`notify_all`）。单次事件用它纯属自找麻烦。

## 方式三：C++20 std::latch（N 个事件的汇聚）

`promise/future` 是 1 对 1 的单次信号；要等"**3 个初始化线程全部就绪**"这种 N 对 1 汇聚，C++20 的 `latch` 是专用原语（clang 23.1.0 + `-std=c++20` 实测运行通过）：

```cpp
#include <latch>

std::latch init_done(3);                    // 计数 3
std::vector<std::thread> ts;
for (int i = 0; i < 3; ++i)
    ts.emplace_back([&, i]{
        initConnection(i);                  // 各自的初始化
        init_done.count_down();             // 完成一个，计数 -1
    });
init_done.wait();                           // 阻塞到计数归零
// 全部就绪
for (auto& t : ts) t.join();
```

无锁、无虚假唤醒、语义直白。`latch` 是一次性的（归零后不能复位）；要多轮同步用 `std::barrier`。

## 赠品：异常穿越线程边界

cv + flag 只能表达"发生了/没发生"；promise 还能把**异常**快递给等待方（实测）：

```cpp
std::promise<void> p;
auto fut = p.get_future();
std::thread t([&]{
    try {
        connectToExchange();        // 抛 std::runtime_error("连接交易所失败")
    } catch (...) {
        p.set_exception(std::current_exception());   // 打包异常
    }
});
try {
    fut.get();                      // 异常在这里重新抛出
} catch (const std::exception& e) {
    // 实测输出：主线程捕获: 连接交易所失败
}
t.join();
```

工作线程的异常不会死在没人接的角落里——这是初始化场景的关键能力（初始化失败的**原因**必须传到主线程）。

---

## 常见错误（新手踩坑）

1. **cv 裸 wait 不带谓词**。`cv.wait(lk);` 在虚假唤醒后直接往下跑——谓词形式 `cv.wait(lk, pred)` 是唯一正确写法。
2. **flag 不用锁保护或不用原子**。`bool ready` 被两个线程一读一写是数据竞争（UB）——cv 方案的 flag 必须在 mutex 保护下读写。
3. **promise 二次 set_value**。`promise` 只能兑现一次，第二次 `set_value` 抛 `std::future_error`——这正是"单次"语义的强制体现。
4. **`future.get()` 当轮询用**。`get()` 是**一次性消费**——拿完 future 就失效，第二次 `get()` 行为未定义。要能反复查结果用 `shared_future`。

---

## 新手要点

- **选型决策树**：单次 1 对 1 → `promise/future`；单次 N 对 1 汇聚 → C++20 `latch`（老标准就 N 个 promise 或原子计数器 + 最后一个负责 notify）；重复事件 / 广播 → `condition_variable`。
- **能拿 C++20 就用 latch**：`std::latch` 是本场景最简洁的标准原语，比手写"计数器 + mutex + cv"少一整类错误。
- **promise/future 的共享状态有堆分配开销**——所以这是"初始化/启动"级工具，不是每 tick 用的热路径工具。

---

## HFT 关联

- **启动序列**：主线程等网络线程完成交易所连接 + 合约表下载后才开闸下单——`promise<void>`（或要带回连接句柄就 `promise<Session*>`）是标准写法；失败原因用 `set_exception` 带回主线程（实测异常穿越可用）。
- **多通道就绪汇聚**：等 3 个行情通道 + 2 个下单通道全部就绪 → `std::latch ready(5)`，每个通道初始化完 `count_down()`，主循环 `wait()` 后开工。
- **别用在热路径**：promise/future 的共享状态带堆分配和原子操作——它管"启动"不管"每 tick"。热路径的线程间通信是无锁队列（SPSC/MPMC ring buffer），不是 future。

---

## 自测题

1. `promise + future` 相比 `condition_variable + flag` 有哪些优势？（至少说出四条）
2. 虚假唤醒是什么？为什么 `cv.wait(lk, pred)` 的谓词形式是底线？promise/future 为什么免疫？
3. 丢失唤醒是什么场景？promise 靠什么机制免疫它？
4. `std::latch` 解决什么问题？和 `std::barrier` 的区别？
5. 工作线程的异常怎么传到等待线程？写出关键两行。
6. `future.get()` 能调用两次吗？要多次读取结果该用什么？

<details>
<summary>参考答案</summary>

1. ①代码极简：`set_value` + `get` 两行，不需要 flag/mutex/谓词；②免疫虚假唤醒：`get()` 只在共享状态真正就绪时返回；③免疫丢失唤醒：共享状态持久，`set_value` 先于 `get` 也不会丢；④少一把锁：共享状态自身线程安全；⑤能捎带结果（`promise<int>`）；⑥能传播异常（`set_exception`）。

2. 虚假唤醒：标准允许 `wait` 在没有任何 notify 的情况下返回（实现策略）。所以等待方醒来后**必须重新检查条件**——`cv.wait(lk, pred)` 等价于 `while (!pred()) wait(lk);`，虚假唤醒会被谓词挡回去再睡。裸 `wait(lk)` 不加检查就会在虚假唤醒时带着未就绪的状态继续跑。promise/future 免疫是因为 `get()` 没有"先醒来再检查"的中间态，标准保证它只在就绪时返回。

3. 通知方在等待方进入 `wait` 之前就 `notify`——通知没人接收就丢了，等待方睡到永远。cv 方案靠"flag 已在锁内置位、wait 前先查谓词"兜底；promise 的共享状态是持久的：`set_value` 把结果写进共享状态，之后任何时刻 `get()` 都能取到——事件被**记录**而不是被**广播**，从机制上不存在错过。

4. `latch` 是一次性的向下计数同步点：构造给 N，参与方各 `count_down()`，等待方 `wait()` 到归零。解决"等 N 个事件/线程全部完成"。与 `barrier` 的区别：`latch` 归零后不可复位（一次性），`barrier` 可重复用于多轮同步；`latch` 的等待方不必是参与方（一个线程等、N 个线程降计数），`barrier` 是所有参与方同时到达后一起放行。

5. `p.set_exception(std::current_exception());`（工作线程 catch 块里打包）+ 等待方 `fut.get()`（异常在此处重抛，用 try/catch 接住）。实测：工作线程抛 `runtime_error("连接交易所失败")`，主线程 `fut.get()` 处原样捕获。

6. 不能。`get()` 是一次性消费：取走结果后 future 失效，第二次 `get()` 行为未定义（通常抛 `std::future_error`）。要多个等待方/多次读取，用 `promise` 配 `shared_future`（`p.get_future().share()`）——`shared_future::get()` 返回 const 引用，可多次、可多方。

</details>

---

## 参考与延伸

- 下一节：[Item 40 atomic vs volatile](item40-atomic-vs-volatile.md)
- 回到：[第 7 章](README.md)
