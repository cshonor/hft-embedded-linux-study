# Item 36：明确指定启动策略

> 第 7 章 并发 API · Item 36 · 上一节：[Item 35 async vs thread](item35-async-vs-thread.md)

## 为什么要学这个（先建立直觉）

C 程序员的并发直觉来自 `pthread_create`：调用返回时，新线程**一定**已经开始跑了。把这套直觉带进 `std::async` 会踩到 C++ 并发 API 最隐蔽的坑：`std::async(f)` 的默认启动策略不是「异步执行」，而是 `std::launch::async | std::launch::deferred`——**标准允许实现二选一**，选哪个由运行时说了算。

如果运行时选了 `deferred`，你的任务根本不会在新线程上跑。它被推迟到你对 future 调用 `get()` 或 `wait()` 的那一刻，在**调用方的线程**里同步执行。你以为「这事在后台跑着呢」，实际它安安静静躺在 future 里睡大觉；如果始终没人 `get()`，它就**永远不执行**。

这个行为因平台、因负载而异，难以复现。防御方法只有一条：把策略写出来。要么 `std::launch::async`（真异步），要么 `std::launch::deferred`（明说我要延迟），别让运行时替你掷骰子。

---

## 这节讲什么

1. 默认策略 `async | deferred` 的语义：实现可二选一；
2. **deferred 实测**：不 `get()`/`wait()` 就不执行，`wait_for(0)` 返回 `future_status::deferred`；
3. **强制 `launch::async`**：没 `get()` 任务已在跑（实测）；
4. deferred 在 GUI/事件循环里的**饥饿陷阱**，以及本机默认策略的实际选择。

---

## 默认策略：运行时可以二选一

```cpp
auto fut = std::async([]{ return heavy_work(); });
// 等价于 std::async(std::launch::async | std::launch::deferred, ...)
// 标准允许实现：
//   1. 立即在新线程执行（async）
//   2. 推迟到 get()/wait() 才在调用方线程同步执行（deferred）
// 你无法从调用代码看出选了哪条！
```

两条路的行为差异是本质性的：async 下任务与调用方**并发**；deferred 下任务与调用方**串行**，且执行时机被推迟到 future 被取值的那一刻。「默认策略」把并发语义变成了实现细节——这正是要避开它的原因。

---

## deferred 实测：不 get 就不跑

显式指定 `launch::deferred`，用标志位观察任务是否真的执行过：

```cpp
#include <chrono>
#include <future>
#include <iostream>
#include <thread>

int main() {
    bool ran = false;
    auto fut = std::async(std::launch::deferred, [&] {
        ran = true;
        std::cout << "任务体执行\n";
        return 42;
    });

    std::this_thread::sleep_for(std::chrono::milliseconds(200));
    std::cout << "创建 200ms 后 ran = " << ran << "（任务没跑）\n";

    // wait_for(0) 检测 deferred
    auto st = fut.wait_for(std::chrono::seconds(0));
    std::cout << "wait_for(0) == future_status::deferred ? "
              << (st == std::future_status::deferred) << "\n";
    std::cout << "检测后 ran = " << ran << "（wait_for 不触发执行）\n";

    std::cout << "get() = " << fut.get() << "\n";
    std::cout << "get 之后 ran = " << ran << "\n";
    return 0;
}
```

本机实测（clang 23.1.0，`-std=c++17 -Wall`）输出：

```
创建 200ms 后 ran = 0（任务没跑）
wait_for(0) == future_status::deferred ? 1
检测后 ran = 0（wait_for 不触发执行）
get() = 任务体执行
42
get 之后 ran = 1
```

三个要点全被坐实：①创建后等了 200ms，`ran` 仍是 0——任务根本没跑；②`wait_for(0)` 返回 `future_status::deferred`——这是检测 deferred 的标准手法，且 `wait_for` **不会**触发执行；③`get()` 的那一刻任务才在**当前线程**同步执行（注意输出顺序：`get() = ` 之后才是 `任务体执行`，然后才是返回值 `42`）。

---

## 强制 launch::async：真异步

```cpp
#include <chrono>
#include <future>
#include <iostream>
#include <thread>

int main() {
    bool ran = false;
    auto fut = std::async(std::launch::async, [&] {
        ran = true;
        return 42;
    });
    std::this_thread::sleep_for(std::chrono::milliseconds(100));
    std::cout << "100ms 后 ran = " << ran << "（还没 get 就已执行）\n";
    std::cout << "get() = " << fut.get() << "\n";

    // 对照：本机默认策略实际选了哪条？
    bool ran2 = false;
    auto fut2 = std::async([&] { ran2 = true; return 1; });   // 默认策略
    std::this_thread::sleep_for(std::chrono::milliseconds(100));
    std::cout << "默认策略 100ms 后 ran2 = " << ran2 << "\n";
    std::cout << "fut2.get() = " << fut2.get() << "\n";
    return 0;
}
```

本机实测（clang 23.1.0）输出：

```
100ms 后 ran = 1（还没 get 就已执行）
get() = 42
默认策略 100ms 后 ran2 = 1
fut2.get() = 1
```

`launch::async` 下任务在 100ms 内已在别的线程跑完，没等 `get()`。本机（libc++）的默认策略这次选了 async（`ran2 = 1`）——**但这是实现的当前选择，不是标准保证**。换 STL（MSVC）、换负载、换版本都可能选 deferred；依赖默认策略就是把正确性押在你控制不了的变量上。

三种策略对比：

| 策略 | 行为 | 任务何时执行 | 适用场景 |
|------|------|--------------|----------|
| `launch::async` | 强制新线程异步执行 | `async` 调用后立即 | 真并发任务 |
| `launch::deferred` | 推迟到 `get()`/`wait()` 在调用方线程同步执行 | 取值那一刻；无人取值则永不执行 | 惰性求值 |
| 默认（`async \| deferred`） | 实现二选一 | 不确定 | **不要用** |

---

## deferred 的饥饿陷阱（场景描述）

deferred 最毒的用法是配 `wait_for` 轮询：

```cpp
auto fut = std::async([]{ return heavy_work(); });   // 若被 deferred……
while (fut.wait_for(std::chrono::milliseconds(100)) != std::future_status::ready) {
    do_ui_frame();   // GUI 主线程继续渲染
}
// 循环永远出不来：deferred 任务只有 get()/wait() 才执行，
// wait_for 不算数（实测）→ ready 永远等不到 → 死循环
```

在 GUI / 事件循环线程上这就是饥饿：事件循环每转一圈都检查一次 future，future 永远不是 ready（deferred 任务在等待一个永远不会来的 `get()`），界面卡死但 CPU 占用看起来正常。同类事故在交易系统的监控线程里同样成立：「等后台任务完成再上报」的轮询循环，遇到 deferred 就是无限等待。防御手法是检测后补救：

```cpp
if (fut.wait_for(std::chrono::seconds(0)) == std::future_status::deferred) {
    // 被 deferred 了：要么就地同步执行，要么改用 launch::async 重发任务
    fut.wait();   // wait() 会触发 deferred 任务执行
}
```

但更干净的做法仍是源头控制：创建时就写 `std::launch::async`。

---

## 完整可运行例子

两种策略并排对照（上文两段实测代码的精简合体）：

```cpp
#include <chrono>
#include <future>
#include <iostream>
#include <thread>

int main() {
    bool a = false, d = false;

    auto fa = std::async(std::launch::async, [&] { a = true; return 1; });
    auto fd = std::async(std::launch::deferred, [&] { d = true; return 2; });

    std::this_thread::sleep_for(std::chrono::milliseconds(100));
    std::cout << "100ms 后：async 任务已执行?" << a
              << "  deferred 任务已执行?" << d << "\n";
    std::cout << "fd.wait_for(0) 是 deferred? "
              << (fd.wait_for(std::chrono::seconds(0))
                  == std::future_status::deferred) << "\n";
    std::cout << "fa.get()=" << fa.get() << "  fd.get()=" << fd.get() << "\n";
    std::cout << "get 之后：d = " << d << "\n";
    return 0;
}
```

本机实测（clang 23.1.0，`-std=c++17 -Wall`）输出：

```
100ms 后：async 任务已执行?1  deferred 任务已执行?0
fd.wait_for(0) 是 deferred? 1
fa.get()=1  fd.get()=2
get 之后：d = 1
```

同一行时间点上两个任务状态相反：`async` 已跑完，`deferred` 纹丝未动——直到 `fd.get()` 才执行。

---

## 常见错误（新手踩坑）

1. **用默认策略以为「一定在后台跑」**。默认是 `async | deferred`，实现可二选一；若被 deferred，`fut.get()` 那一刻任务在**你的线程**同步执行，热路径当场阻塞一次完整计算。修正：显式写 `std::launch::async`。
2. **用 `wait_for(0) == ready` 检测 deferred**。deferred 任务永远不报 ready——检测值应是 `std::future_status::deferred`（实测返回的正是它）。`wait_for`/`wait_for(0)` 都不会触发 deferred 任务执行（实测）。
3. **deferred 任务配 `wait_for` 轮询**。循环条件 `wait_for(...) != ready` 永远成立——死循环（GUI 饥饿陷阱，见上节）。修正：检测到 deferred 后 `wait()`（会触发执行），或源头改用 `launch::async`。
4. **显式 `launch::async` 后忽略资源失败**。强制 async 在系统线程资源耗尽时抛 `std::system_error`；高并发场景要 catch 住并降级（比如回落到同步执行）。

---

## HFT 关联

- **延迟确定性**：HFT 最怕「不可预测的同步阻塞」——默认策略下一个本该后台跑的风控检查可能被推迟到 `get()` 时在关键线程同步执行，造成延迟尖峰。规则：**所有 `std::async` 调用必须带显式策略**，代码评审 grep `std::async(` 后面跟的不是 `std::launch` 的全部打回。
- **后台任务用 `launch::async`**：日志落盘、快照上传、异步风控——`std::async(std::launch::async, ...)` 保证调用后立刻离开当前线程。
- **配置懒加载可用 `launch::deferred`**：启动时不算、第一次用到才算的开销（如非关键路径的模型参数），用 deferred 明说「惰性求值」意图——但必须确保取值点不在热路径上。
- **事件循环线程警惕 deferred**：网关的 epoll 循环、GUI 监控线程里若用 future 轮询等待默认策略的 async 任务，撞上 deferred 就是死等——检测 `future_status::deferred` 或干脆只在事件循环外用 async。

---

## 自测题

**1. `std::async(f)` 的默认启动策略是什么？为什么它危险？**
<details><summary>答案</summary>
默认策略是 `std::launch::async | std::launch::deferred`——标准允许实现**二选一**，由运行时决定。若选了 deferred，任务不立即执行，而是推迟到 `get()`/`wait()` 时在调用方线程同步执行；始终不取值则永不执行。危险在于并发语义变成了实现细节：因平台、负载而异，难以复现，「以为在后台跑实际同步阻塞」的 bug 无法从调用代码看出。
</details>

**2. 如何检测一个 future 对应的任务被 deferred 了？检测本身会触发任务执行吗？**
<details><summary>答案</summary>
用 `fut.wait_for(std::chrono::seconds(0))`，返回 `std::future_status::deferred` 即为 deferred（真异步任务会返回 `ready` 或 `timeout`）。本文实测：deferred 任务创建 200ms 后仍未执行，`wait_for(0)` 返回 deferred，且检测后任务依然没跑——`wait_for` 不会触发执行，只有 `get()`/`wait()` 会。
</details>

**3. deferred 任务在什么确切时机执行？在哪个线程？**
<details><summary>答案</summary>
在对返回的 future 首次调用 `get()` 或 `wait()` 的那一刻执行，在**发起该调用的线程**上同步执行（不是新线程）。本文实测输出顺序 `get() = 任务体执行 → 42` 证明执行夹在 `get()` 调用之中。若始终无人 `get()`/`wait()`，任务永不执行。
</details>

**4. 为什么 GUI/事件循环里 deferred 会导致饥饿？**
<details><summary>答案</summary>
典型写法是 `while (fut.wait_for(...) != ready) { 继续处理事件; }`。deferred 任务只有 `get()`/`wait()` 才执行，`wait_for` 不算——所以 future 永远不会变成 ready，循环永远出不来：事件循环卡死，任务也永远没跑，界面冻结但 CPU 占用正常。本文实测确认 `wait_for` 不触发 deferred 任务。修正：检测到 deferred 后用 `wait()`，或源头显式 `launch::async`。
</details>

**5. 强制真异步的正确写法是什么？它可能抛什么异常？**
<details><summary>答案</summary>
`auto fut = std::async(std::launch::async, f, args...);`——保证任务在新线程异步执行（本文实测：100ms 后未 `get()` 任务已执行完）。若系统线程资源耗尽导致无法创建新执行上下文，抛 `std::system_error`；高并发场景应 catch 并降级（如回落到同步执行）。
</details>

**6. 本机实测默认策略选了 async，能否据此在代码里放心用默认策略？**
<details><summary>答案</summary>
不能。本机（clang 23.1.0 + libc++）这次默认策略选了 async（实测 `ran2 = 1`），但这只是该实现的当前选择——标准明文允许二选一，换标准库实现（libstdc++、MSVC STL）、换版本、换负载都可能变成 deferred。把正确性押在实现细节上是隐患；显式写出策略才能让行为成为代码的一部分。
</details>

---

## 参考与延伸

- 上一节：[Item 35 async vs thread](item35-async-vs-thread.md)
- 下一节：[Item 37 thread joinable](item37-thread-joinable.md)
- 回到：[第 7 章 并发 API](README.md)
