# Item 38：理解线程句柄的析构与联结行为

> 第 7 章 并发 API · Item 38 · 上一节：[Item 37 thread joinable](item37-thread-joinable.md) · 下一节：[Item 39 单次事件](item39-one-shot-events.md)

## 为什么要学这个（先建立直觉）

`std::async` 的字面意思是"异步"——你发起一个任务，拿到一个 `future`，然后去忙别的。但下面这段代码的行为和直觉完全相反：

```cpp
{
    auto fut = std::async(std::launch::async, []{ long_task(); });
    // 没调用 get()，没调用 wait()，你以为 fut 析构就是扔掉句柄
}  // 实际：程序在这一行阻塞，直到 long_task 跑完
```

你以为写了个"发射后不管"的异步调用，编译器却在 `fut` 的析构点悄悄帮你 `join` 了那个线程。这个阻塞出现在一个**完全没有任何函数调用痕迹**的地方——只是离开了一个作用域。在热路径上，这就是一次无法从代码表面看出来的同步卡顿。

更麻烦的是，这条规则**不是**所有 `future` 都遵守：只有 `std::async`（非 deferred）创建的那一份共享状态才有"最后一个句柄析构时阻塞"的特殊待遇，`std::packaged_task` 和 `std::promise` 产生的 `future` 析构时抬腿就走。不搞清楚边界，你既会在不该卡的地方卡住，也会在以为有阻塞保证的地方失去保证。

---

## 这节讲什么

`future` 析构的阻塞规则及其精确边界：**只对 `std::async`（`launch::async`）创建的共享状态生效，且只在最后一个引用该共享状态的句柄被释放时触发**——相当于隐式 join。本机实测用 chrono 计时证明：async 的 future 析构阻塞约等于任务时长，packaged_task / promise 的 future 析构零等待。

---

## 规则本身：什么时候阻塞，什么时候不阻塞

| 共享状态的来源 | 最后一个句柄析构时 | 类比 |
|----------------|-------------------|------|
| `std::async` + `launch::async` | **阻塞**，直到任务完成（隐式 join） | 隐形的 `t.join()` |
| `std::async` + `launch::deferred` | 不阻塞（任务根本没启动） | 惰性求值被丢弃 |
| `std::packaged_task::get_future()` | 不阻塞 | 普通句柄 |
| `std::promise::get_future()` | 不阻塞 | 普通句柄 |

标准的措辞是：async（async 策略）创建的异步任务，其线程完成 synchronizes-with "第一个成功观察到共享状态就绪的函数的返回"或"**最后一个释放该共享状态的函数的返回**"，两者取先发生者。所以触发阻塞的不是"future 这个类"，而是"async 共享状态的最后一次释放"——调用 `get()`/`wait()` 已经观察到就绪的话，之后的析构自然不再等。

这也解释了为什么阻塞存在：async 任务可能按引用捕获了调用者的局部变量，如果句柄析构时拍屁股走人，任务就变成了野线程。这个隐式 join 和 [Item 37](item37-thread-joinable.md) 的 terminate 是同一种思路——标准库不允许你"忘记"一个正在跑的线程。

---

## 实测：四种来源的 future 析构计时（本机实测 clang 23.1.0）

任务固定 sleep 300ms，所有 future 都**不调 `get()`/`wait()`**，只在作用域结束处析构，用 chrono 量作用域退出时刻：

```cpp
#include <chrono>
#include <cstdio>
#include <future>
#include <thread>

using Clock = std::chrono::steady_clock;
using Ms = std::chrono::milliseconds;

long long elapsed_ms(Clock::time_point t0) {
    return std::chrono::duration_cast<Ms>(Clock::now() - t0).count();
}

int slow_task() {
    std::this_thread::sleep_for(Ms(300));
    return 42;
}

int main() {
    // ---- 1. async(launch::async)：析构阻塞 ----
    {
        auto t0 = Clock::now();
        {
            std::future<int> fut = std::async(std::launch::async, slow_task);
            std::printf("[async ] about to leave scope at t=%lldms\n", elapsed_ms(t0));
        }  // fut 析构 → 阻塞等 slow_task 跑完
        std::printf("[async ] scope exited at t=%lldms\n\n", elapsed_ms(t0));
    }

    // ---- 2. packaged_task：析构不阻塞 ----
    {
        auto t0 = Clock::now();
        {
            std::packaged_task<int()> pt(slow_task);
            std::future<int> fut = pt.get_future();
            std::thread t(std::move(pt));
            t.detach();
        }  // fut 析构 → 立即返回
        std::printf("[ptask ] scope exited at t=%lldms\n\n", elapsed_ms(t0));
    }

    // ---- 3. promise：析构不阻塞 ----
    {
        auto t0 = Clock::now();
        {
            std::promise<int> p;
            std::future<int> fut = p.get_future();
            std::thread t([pr = std::move(p)]() mutable {
                std::this_thread::sleep_for(Ms(300));
                pr.set_value(42);
            });
            t.detach();
        }
        std::printf("[promis] scope exited at t=%lldms\n\n", elapsed_ms(t0));
    }

    // ---- 4. async 转 shared_future（最后一个引用）：仍然阻塞！----
    {
        auto t0 = Clock::now();
        {
            std::shared_future<int> sfut =
                std::async(std::launch::async, slow_task).share();
        }  // sfut 是该共享状态最后的引用 → 析构仍阻塞
        std::printf("[shared] scope exited at t=%lldms\n\n", elapsed_ms(t0));
    }

    // ---- 5. 对照：get() 显式等待的耗时 ----
    {
        auto t0 = Clock::now();
        int v = std::async(std::launch::async, slow_task).get();
        std::printf("[get() ] got %d at t=%lldms\n", v, elapsed_ms(t0));
    }

    std::this_thread::sleep_for(Ms(500));  // 等后台 detached 线程收尾
    return 0;
}
```

clang 23.1.0 + `-std=c++17 -Wall` 实测输出：

```
[async ] about to leave scope at t=0ms
[async ] scope exited at t=310ms

[ptask ] scope exited at t=0ms

[promis] scope exited at t=0ms

[shared] scope exited at t=304ms

[get() ] got 42 at t=301ms
```

四组对照把规则边界钉死了：

1. **async 的 future 析构阻塞 310ms**——和 `get()` 的 301ms 一样，隐式 join 货真价实；
2. **packaged_task / promise 的 future 析构 0ms**——线程还在后台跑，句柄先走了，互不等待；
3. **shared_future 也阻塞了 304ms**——这是最容易记错的一点：阻塞规则挂在"共享状态的最后一次释放"上，`.share()` 只是换了句柄类型，共享状态还是 async 创建的，最后一个 `shared_future` 析构照样等。想真正躲开阻塞，办法不是换类型，而是**让句柄活得比当前作用域久**（存进成员变量、队列、后台收割线程）。

---

## 推论：临时 future 立即析构 = 假异步

```cpp
std::async(std::launch::async, long_task);   // 返回值被丢弃
// 临时 future 在语句末尾立即析构 → 立即阻塞 → 完全同步，白 async 了
```

这是这条规则最坑的形态：丢弃返回值的 `std::async` 调用**等价于直接同步调用**，还额外付出了一次线程启动的开销。如果你真的想"发射后不管"，得用 `std::thread` + detach（并自担悬垂风险），或把 future 移交给他处保管。

---

## 完整可运行例子

上方"实测"一节即完整可运行程序：五种来源各测一次，输出已给出。编译命令：

```
clang++ -std=c++17 -Wall test.cpp   # 本机实测 clang 23.1.0，线程代码无需 -pthread
```

---

## 常见错误（新手踩坑）

**错误 1：在热路径析构 async 的 future**

```cpp
void on_tick(const Tick& t) {
    auto fut = std::async(std::launch::async, check_risk, t);
    process(t);
}   // fut 在函数末尾析构 → 阻塞等 check_risk 完成 → 热路径卡住
```

**修正：** 把 future 存进成员队列，由后台线程收割 `get()`。

**错误 2：丢弃 `std::async` 返回值，以为实现了异步**

```cpp
std::async(std::launch::async, long_task);   // 临时 future 立即析构 → 同步阻塞
```

**修正：** `auto fut = std::async(...);` 存起来并妥善安排生命周期。

**错误 3：以为所有 future 析构都阻塞（或都不阻塞）**

```cpp
// packaged_task 的 future 析构不阻塞——任务结果可能永远没人取
// async 的 future 析构阻塞——"扔掉句柄"不等于"取消任务"
```

**修正：** 记住规则的精确边界：只有 async 共享状态的**最后一次释放**才阻塞。

**错误 4：以为 `.share()` 成 shared_future 就能躲开析构阻塞**

**修正：** 实测证明最后一个 `shared_future` 析构照样等任务跑完；要躲开阻塞得延长句柄生命周期，不是换类型。

---

## 新手要点（和 C 的区别）

| 维度 | C 怎么做 | C++ 怎么做 | 为什么 |
|------|---------|-----------|--------|
| 异步任务句柄 | `pthread_t` 整数，离开作用域无效果 | `future` 析构可能隐式 join | RAII + 共享状态 |
| "发射后不管" | 不 join 即可 | 丢弃 async 返回值反而同步阻塞 | 析构语义反直觉 |
| 结果回收 | 全局变量 + 标志位轮询 | `future::get()` 一次性取走 | 共享状态自动管理 |

**一句话总结：** `std::async` 的 future 析构 = 隐式 join（仅当它是该共享状态的最后一个引用且任务未就绪）；packaged_task / promise 的 future 析构无此待遇。

---

## HFT 关联

- **热路径零隐式阻塞**：`on_tick` / `on_order` 回调里出现的任何 async future 都必须移交出去（队列、成员变量），否则析构阻塞直接变成逐笔延迟。
- **异步风控 / 异步落盘**：发射即忘的任务别用 `std::async`（返回值一丢就同步），用线程池或 `std::thread`+detach+按值捕获。
- **后台收割线程**：统一一个 reaper 线程对消费者队列里的 future 调 `get()`——既避开析构阻塞，也避免任务异常被静默吞掉。
- **定位神秘卡顿**：热路径出现与某任务时长吻合的延迟尖刺时，grep 作用域内是否有 async future 的析构点。

---

## 自测题

**1. `std::future` 的析构什么时候会阻塞？精确的触发条件是什么？**

<details><summary>答案</summary>
仅当同时满足：①共享状态由 `std::async` 以 `launch::async` 策略创建；②任务尚未就绪；③这是最后一个引用该共享状态的句柄（future 或 shared_future）被释放。此时析构相当于隐式 join，阻塞到任务完成。packaged_task / promise 的 future 永远不阻塞。
</details>

**2. 实测中四种来源的 future 析构耗时各是多少？说明了什么？**

<details><summary>答案</summary>
async future 310ms（阻塞，≈任务时长 300ms）；packaged_task 0ms；promise 0ms；shared_future（最后引用）304ms。说明阻塞规则挂在"async 共享状态的最后一次释放"上，与句柄是 future 还是 shared_future 无关。
</details>

**3. `std::async(std::launch::async, task);`（丢弃返回值）的实际行为是什么？**

<details><summary>答案</summary>
临时 future 在语句末尾立即析构，按规则阻塞到 task 完成——等价于同步调用，还白付一次线程启动开销。不是"发射后不管"。
</details>

**4. 为什么 `packaged_task` 和 `promise` 的 future 析构不阻塞？**

<details><summary>答案</summary>
阻塞是标准专门给 async（async 策略）共享状态加的"线程完成 synchronizes-with 最后一次释放"条款，目的是防止 async 任务变野线程。packaged_task / promise 的线程生命周期由你自己管理（你自己创建并 join/detach 了 `std::thread`），标准不再插手。
</details>

**5. 已经调用过 `get()` 的 async future，析构还阻塞吗？为什么？**

<details><summary>答案</summary>
不阻塞。`get()` 返回意味着共享状态已就绪——"第一个成功观察到就绪的函数"已经发生，按"两者取先"的规则，之后的释放不再承担等待义务（且 `get()` 后 future 已 `valid()==false`，不再引用共享状态）。
</details>

**6. 要真正避免 async future 的析构阻塞，正确做法是什么？**

<details><summary>答案</summary>
延长句柄生命周期：存进成员变量 / 队列 / 后台收割线程，让"最后一次释放"发生在不敏感的地方。换成 shared_future 无效——实测证明它作为最后引用照样阻塞。
</details>

**7. 这条析构阻塞规则与 Item 37 的 terminate 有什么共同的设计动机？**

<details><summary>答案</summary>
都是"不允许你忘记一个正在运行的线程"：Item 37 用 terminate 惩罚忘记 join 的 `std::thread`，Item 38 用隐式 join 防止 async 任务变成没人管的野线程（它可能按引用捕获了调用者的局部变量）。一个用崩溃保证，一个用阻塞保证。
</details>

---

## 参考与延伸

- 上一节：[Item 37 thread joinable](item37-thread-joinable.md)
- 下一节：[Item 39 单次事件](item39-one-shot-events.md)
- 回到：[第 7 章 并发 API](README.md)
