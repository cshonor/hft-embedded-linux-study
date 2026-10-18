# Item 39：单次事件用 future + promise / condition_variable

> 第 7 章 并发 API · Item 39 · 上一节：[Item 38 句柄析构行为](item38-handle-destruction.md) · 下一节：[Item 40 atomic vs volatile](item40-atomic-vs-volatile.md)

## 为什么要学这个（先建立直觉）

并发编程里最高频的协作模式之一是："线程 B 必须等线程 A 完成**一次**某件事"——等连接建好、等配置加载完、等行情首包到达。教科书的标准答案是 `condition_variable` + flag + mutex 三件套，但这三件套有两颗经典地雷（虚假唤醒、丢失唤醒），每颗都足以让新手写下"九成九能跑"的代码。

`promise` + `future` 把同一个模式压缩成两行，而且**从机制上免疫**那两颗地雷。Meyers 的立场很明确：单次事件，默认用 promise/future；condition_variable 留给真正需要它的重复事件。

---

## 这节讲什么

1. promise/future 单次通信（实测等待耗时），以及它为什么免疫虚假唤醒与丢失唤醒；
2. `condition_variable` + flag 的正解写法与两颗地雷的机理（含实测）；
3. C++20 `latch`（N 对 1）与 `barrier`（N 对 N 多轮）；
4. `shared_future` 一对多广播、promise 的"单次"语义、`get()` 的一次性；
5. 异常穿越线程边界；
6. 选型决策树，以及轮询 / 信号量的取舍（含轮询实测）。

本机环境：clang 23.1.0，arm64-apple-darwin，libc++，`-std=c++17 -Wall`（C++20 段落标 `-std=c++20`），线程代码本机无需 `-pthread`。

---

## 方式一：promise + future（单次事件首选）

```cpp
std::promise<std::string> p;
auto fut = p.get_future();
std::thread net([&]{ p.set_value(connect(50)); });   // 建连成功后发信号并捎带结果
std::string s = fut.get();                           // 阻塞，直到 set_value 发生
net.join();
```

clang 23.1.0 实测（这里的 `connect` 睡 50 ms）：

```
[1] 拿到 session-established，等待 60.17 ms
```

### 为什么它免疫两颗地雷

| 地雷 | cv 方案 | promise/future |
|---|---|---|
| **丢失唤醒**（notify 早于 wait） | 靠 flag 已在锁内置位兜底；裸 `wait` 会永远错过 | 共享状态**持久**：`set_value` 把结果写进去，之后任何时刻 `get()` 都能取到，事件被**记录**而非**广播** |
| **虚假唤醒**（没人 notify 却醒了） | 必须带谓词 `wait(lk, pred)` 循环重新检查 | `get()` 没有"先醒来再检查"的中间态，标准保证只在就绪时返回 |

此外它还**少一把锁**（cv 方案必须 mutex 保护 flag，promise 的共享状态自身线程安全），并附赠携带结果与传播异常两项能力。代价是共享状态通常带一次堆分配 + 原子操作——**这是"初始化/启动"级工具，不是每 tick 的热路径工具**。

---

## 方式二：condition_variable + flag（重复事件的唯一选择）

```cpp
// 通知方
{
    std::lock_guard<std::mutex> lk(m);
    pending += 1;            // ★ 谓词保护的状态必须在锁内写
}
cv.notify_all();
// 等待方
std::unique_lock<std::mutex> lk(m);
cv.wait(lk, [&]{ return pending != 0; });   // ★ 谓词形式是底线，不是优化
--pending;
```

### 两颗地雷的机理（实测）

**丢失唤醒**——通知在等待之前到达，实测对照：

```
谓词形式 wait_for 是否立刻通过: 是
裸 wait_for 是否通过（该通知已被丢弃）: 否，超时
```

构造：先把 flag 置位并 `notify_one()`，此后再让等待方进入等待。谓词形式**先检查 flag**（事实已被记录）→ 立刻通过；裸 `wait_for` 依赖那条已经消逝的 notify → 超时。**notify 只负责叫醒，真正兜底的是锁内的 flag。**

**虚假唤醒**——让通知线程连发 5 次"噪声" notify（谓词始终为假）后再真正置位，实测：

```
谓词被检查 7 次，其中 6 次是假唤醒（被谓词挡回去）
```

`cv.wait(lk, pred)` 等价于 `while (!pred()) wait(lk);`。实测里谓词被调用 7 次、6 次挡回去。**裸 `wait(lk)` 加事后不检查 flag，就会在虚假唤醒时带着未就绪的状态继续跑。**

cv + flag 真正不可替代的场景：**重复事件**（每来一个 tick 通知一次）、**多等待方广播**（`notify_all`）、**带队列的生产者-消费者**。单次事件用它纯属自找麻烦。

---

## 方式三：C++20 latch / barrier

```cpp
std::latch ready(3);                       // 计数 3
for (int i = 0; i < 3; ++i)
    ts.emplace_back([&, i]{ setup(i); ready.count_down(); });
ready.wait();                              // 阻塞到计数归零
```

`latch` 是一次性向下计数门闩，解决"等 N 个事件全部完成"（**N 对 1**，等待方不必是参与方）。实测：

```
A) 通道 0 就绪
A) 通道 1 就绪
A) 通道 2 就绪
A) 全部就绪，主线程开闸
```

`barrier` 是**可复用**的多轮汇合点（**N 对 N**）：所有参与方 `arrive_and_wait()` 到齐才一起放行，然后自动重置进入下一轮。实测 3 个 worker 跑 2 轮：

```
B) worker 0 第 0 轮到达屏障
B) worker 1 第 0 轮到达屏障
B) worker 2 第 0 轮到达屏障
B) worker 2 第 0 轮通过      ← 最后到达却先打印"通过"：放行后的执行顺序不定
B) worker 0 第 0 轮通过   B) worker 1 第 0 轮通过
```

`-std=c++17` 下这一段直接不可用：`error: no member named 'latch' in namespace 'std'`。

---

## 方式四：shared_future 一对多广播 / promise 的单次语义

`future::get()` 是**一次性消费**——要让多个等待方读同一个结果，用 `shared_future`：

```cpp
std::promise<int> p;
auto shared = p.get_future().share();     // move 成 shared_future
for (int i = 0; i < 3; ++i)
    ts.emplace_back([&, i]{ use(shared.get()); });   // 每个线程都能读到
p.set_value(2026);
```

实测 3 个线程全部读到同一个值（输出会因并发而交错，见完整例子）。

**promise 只能兑现一次**——实测第二次 `set_value`：

```
B) 二次 set_value: The state of the promise has already been set.
```

这正是"单次"语义的强制体现，也是 Item 39 标题里 "one-shot" 的由来。

### 诚实实测：`get()` 第二次调用会崩

本机实测需要更正一条流传的说法（"第二次 `get()` 通常抛 `std::future_error`"）：

```
第一次 get = 9
valid = 0                       ← get() 之后 future 立即失效
第二次 get ...                   ← 进程 SIGSEGV（exit code 139），没有任何异常抛出
```

libc++ 的行为是**直接段错误**。所以： **`get()` 只允许一次，二次调用是 UB 且在本机表现为崩溃。** 要么用 `shared_future`，要么在调用前检查 `valid()`。

---

## 赠品：异常穿越线程边界

cv + flag 只能表达"发生了/没发生"；promise 还能把**异常**快递给等待方：

```cpp
std::thread net([&]{
    try { throw std::runtime_error("连接交易所失败"); }
    catch (...) { p.set_exception(std::current_exception()); }   // 打包异常
});
try { fut.get(); }                                              // 异常在此处重新抛出
catch (const std::exception& e) { /* 处理 */ }
```

实测输出 `[2] 主线程捕获: 连接交易所失败`。工作线程的异常不会死在没人接的角落里——初始化失败的**原因**必须能传到主线程，这是初始化场景的关键能力。

---

## 选型决策树与轮询/信号量的取舍

| 场景 | 首选 | 理由 |
|---|---|---|
| 单次 1 对 1 | `promise/future` | 免疫两颗地雷、能带结果/异常、少一把锁 |
| 单次 1 对 N 广播 | `promise` + `shared_future` | `future::get()` 一次性，`shared_future::get()` 可多方多次 |
| 单次 N 对 1 汇聚 | C++20 `latch`（老标准：N 个 promise 或原子计数 + 最后一人 notify） | 语义直白、无锁、无虚假唤醒 |
| N 对 N 多轮同步 | C++20 `barrier` | `latch` 归零后不可复位 |
| 重复事件 / 队列 + 广播 | `condition_variable` + 谓词 | 唯一原生支持重复通知的原语 |
| 计数型资源配额 | C++20 `std::counting_semaphore` / `binary_semaphore` | 表达"还有几份资源"，比 cv 更贴切 |

**轮询**几乎总是错答案。实测等待同一个 80 ms 后发生的事件：

```
C) 轮询：等待 85.77 ms，空转 16388261 次
C) latch：等待 90.05 ms（线程睡眠，不占 CPU）
```

轮询版空转 **1638 万次**、整核 100% 占用；阻塞版线程直接睡过去。**例外**：延迟必须 < 1 µs 且等待极短的自旋场景（如几十纳秒的自旋锁），那时用 `atomic` 自旋 + `yield`，也不是本节的一次性事件。

---

## 完整可运行例子

```cpp
#include <chrono>
#include <condition_variable>
#include <future>
#include <iostream>
#include <latch>
#include <mutex>
#include <stdexcept>
#include <thread>
#include <vector>
using namespace std::chrono;
static std::string connect(int ms) { std::this_thread::sleep_for(milliseconds(ms)); return "session-established"; }
int main() {
    // 1. promise + future：1 对 1 单次事件，带结果
    {
        std::promise<std::string> p;  auto fut = p.get_future();
        auto t0 = steady_clock::now();
        std::thread net([&]{ p.set_value(connect(50)); });
        std::cout << "[1] 拿到 " << fut.get()
                  << "，等待 " << duration<double, std::milli>(steady_clock::now() - t0).count() << " ms\n";
        net.join();
    }
    // 2. 异常穿越线程边界
    {
        std::promise<void> p;  auto fut = p.get_future();
        std::thread net([&]{
            try { throw std::runtime_error("连接交易所失败"); }
            catch (...) { p.set_exception(std::current_exception()); }
        });
        try { fut.get(); }
        catch (const std::exception& e) { std::cout << "[2] 主线程捕获: " << e.what() << "\n"; }
        net.join();
    }
    // 3. cv + flag：重复事件 + 多消费者广播
    {
        std::condition_variable cv; std::mutex m; int pending = 0;
        std::vector<std::thread> consumers;
        for (int i = 0; i < 2; ++i)
            consumers.emplace_back([&, i]{
                for (;;) {
                    std::unique_lock<std::mutex> lk(m);
                    cv.wait(lk, [&]{ return pending != 0; });     // ★ 谓词形式是底线
                    if (pending < 0) break;
                    --pending;
                    lk.unlock();
                    std::cout << "[3] 消费者 " << i << " 取走一个 tick\n";
                }
            });
        std::this_thread::sleep_for(20ms);
        for (int batch = 0; batch < 2; ++batch) {
            { std::lock_guard<std::mutex> lk(m); pending += 2; }
            cv.notify_all();
            std::this_thread::sleep_for(10ms);
        }
        { std::lock_guard<std::mutex> lk(m); pending = -1; }      // 退出哨兵也要满足谓词！
        cv.notify_all();
        for (auto& t : consumers) t.join();
    }
    // 4. latch：N 对 1（需 C++20）
    {
        std::latch ready(3);
        std::vector<std::thread> ts;
        for (int i = 0; i < 3; ++i)
            ts.emplace_back([&, i]{ connect(20 + i * 10); ready.count_down(); });
        ready.wait();
        std::cout << "[4] 3 个通道全部就绪，主线程开闸\n";
        for (auto& t : ts) t.join();
    }
    // 5. shared_future：一对多广播
    {
        std::promise<int> p;  auto shared = p.get_future().share();
        std::vector<std::thread> ts;
        for (int i = 0; i < 3; ++i)
            ts.emplace_back([&, i]{ std::cout << "[5] 线程 " << i << " 读到 set=" << shared.get() << "\n"; });
        p.set_value(2026);
        for (auto& t : ts) t.join();
    }
}
```

本机实测 clang 23.1.0（`-std=c++20 -Wall`）输出（**多线程输出会交错，每次运行排布不同**）：

```
[1] 拿到 session-established，等待 60.17 ms
[2] 主线程捕获: 连接交易所失败
[3] 消费者 0 取走一个 tick
[3] 消费者 0 取走一个 tick
[3] 消费者 [3] 消费者 0 取走一个 tick
1 取走一个 tick
[4] 3 个通道全部就绪，主线程开闸
[5] 线程 0 读到 set=[5] 线程 1 读到 set=[5] 线程 2 读到 set=2026
2026
2026
```

注意后几行**输出是交错的**：三个线程的 `std::cout` 语句互相穿插。`std::cout` 不会撕裂单个 `<<` 操作数，但整条语句不是原子的——演示代码刻意不加锁，生产代码要整齐输出就得额外同步。

---

## 常见错误（新手踩坑）

1. **cv 裸 `wait` 不带谓词**。实测谓词被检查 7 次、其中 6 次是假唤醒，只有 `wait(lk, pred)` 能挡回去。
2. **flag 不用锁保护**。被两个线程一读一写是数据竞争（UB）。cv 方案的谓词状态必须在 mutex 下读写。
3. **以为 notify 自带状态**。实测先 `notify` 后 `wait`，裸版本超时——notify 只负责叫醒，兜底靠 flag。
4. **退出哨兵不满足谓词**。上例把 `pending` 设成 `-1` 时谓词是 `pending != 0` 而不是 `pending > 0`——写成 `> 0` 会让消费者永远醒不过来（实测会挂住不退）。
5. **promise 二次 `set_value`**。实测 `The state of the promise has already been set.`——这就是"one-shot"的强制体现。
6. **`future::get()` 调第二次**。本机 libc++ 实测 **SIGSEGV（139）**，不抛异常。`get()` 后 `valid()` 变 `false`，要多次读取请用 `shared_future`。
7. **拿轮询当同步**。实测空转 1638 万次、整核占满，只换来和阻塞版本差不多的延迟。

---

## HFT 关联

- **启动序列**：主线程等网络线程完成交易所连接 + 合约表下载后才开闸下单——`promise<void>`（或要带回句柄就 `promise<Session*>`）；失败原因用 `set_exception` 带回主线程（实测异常可穿越）。
- **多通道就绪汇聚**：等 3 个行情通道 + 2 个下单通道全部就绪 → `std::latch ready(5)`，每通道初始化完 `count_down()`，主循环 `wait()` 后开工。
- **配置热更新广播**：一次重算要给 N 个策略线程看——`promise<Config>` + `shared_future`，比每人一个 promise 干净，也避开 `get()` 只能一次的坑。
- **别用在热路径**：promise/future 的共享状态带堆分配与原子操作——它管"启动"不管"每 tick"。热路径用无锁队列（SPSC/MPMC ring buffer），重复事件才上 cv。

---

## 自测题

**1. `promise + future` 相比 `condition_variable + flag` 有哪些优势？**
<details><summary>答案</summary>
① 代码极简（`set_value` + `get` 两行，不需要 flag/mutex/谓词）；② 免疫虚假唤醒（`get()` 只在就绪时返回，没有"醒来再检查"的中间态）；③ 免疫丢失唤醒（共享状态持久，先 `set_value` 后 `get` 也不丢）；④ 少一把锁；⑤ 能捎带结果；⑥ 能传播异常。代价：共享状态通常有一次堆分配。
</details>
**2. 虚假唤醒是什么？为什么谓词形式是底线？实测数字是多少？**
<details><summary>答案</summary>
标准允许 `wait` 在没有任何 notify 的情况下返回。所以醒来后必须重新检查条件——`cv.wait(lk, pred)` 等价于 `while (!pred()) wait(lk);`。实测让通知线程连发 5 次"噪声" notify：谓词被检查 7 次，其中 6 次是假唤醒、被谓词挡回去。裸 `wait(lk)` 不检查就会带着未就绪状态继续跑。
</details>
**3. 丢失唤醒是什么？实测中 notify 早于 wait 时会怎样？**
<details><summary>答案</summary>
通知方在等待方进入 `wait` 之前就 `notify`，这次通知没人接收就丢了。实测：先置 flag + `notify_one()`，此后再等待——谓词形式的 `wait_for` **立刻通过**（靠的是"flag 已置位"这个事实），裸 `wait_for` **超时**（通知已消逝）。结论：notify 只负责叫醒，兜底靠锁内的 flag。
</details>
**4. `std::latch` 和 `std::barrier` 的区别？C++17 能用吗？**
<details><summary>答案</summary>
`latch` 是一次性向下计数门闩（N 对 1，等待方可不是参与方），归零后不可复位；`barrier` 是可复用的多轮汇合点（N 对 N），参与方到齐一起放行后自动重置。实测 `-std=c++17` 下 `error: no member named 'latch' in namespace 'std'`。
</details>
**5. 工作线程的异常怎么传到等待线程？**
<details><summary>答案</summary>
工作线程 catch 块里 `p.set_exception(std::current_exception());` 打包，等待方在 `fut.get()` 处异常重抛、用 try/catch 接住。实测输出 `[2] 主线程捕获: 连接交易所失败`。这是初始化场景的关键能力——失败**原因**必须传到主线程，cv + flag 只能表达"发生了/没发生"。
</details>
**6. `future::get()` 能调用两次吗？本机实测是什么结果？**
<details><summary>答案</summary>
不能。`get()` 是一次性消费，返回后 `valid()` 立即变 `false`（实测打印 `valid = 0`）。需要更正流传的说法：本机 libc++ 上第二次 `get()` **直接 SIGSEGV（exit 139），不抛任何异常**——别指望 `catch (std::future_error)`。要么用 `promise` 配 `shared_future`（`get_future().share()`），要么调用前查 `valid()`。
</details>
**7. 为什么轮询几乎总是错答案？实测数据？**
<details><summary>答案</summary>
实测等待同一个 80 ms 后发生的事件：轮询版耗时 85.77 ms、**空转 16388261 次**（整核 100% 占用）；`latch` 阻塞版耗时 90.05 ms、线程直接睡过去不占 CPU。轮询没有任何延迟优势，只是把 CPU 烧掉。例外仅在延迟需 < 1 µs 且等待极短的自旋场景（通常用 atomic 自旋 + yield）。
</details>

---

## 参考与延伸

- 上一节：[Item 38 句柄析构行为](item38-handle-destruction.md)
- 下一节：[Item 40 atomic vs volatile](item40-atomic-vs-volatile.md)
- 相关：[Item 35 async vs thread](item35-async-vs-thread.md)（`std::async` 自带 future，但析构行为不同）
- 相关：[Item 36 launch policy](item36-launch-policy.md)、[Item 37 thread joinable](item37-thread-joinable.md)
- 回到：[第 7 章 并发 API](README.md)
