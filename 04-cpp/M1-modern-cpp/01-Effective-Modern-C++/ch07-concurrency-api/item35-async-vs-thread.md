# Item 35：优先 std::async 而非 std::thread

> 第 7 章 并发 API · Item 35 · 下一节：[Item 36 启动策略](item36-launch-policy.md)

## 为什么要学这个（先建立直觉）

`std::thread` 是「线程制」思维：你向操作系统**要一个线程**，自己管它的生（创建）死（join/detach），它跑什么、什么时候跑、跑完结果给谁，全得你自己搭管道。C 程序员对这个模型很熟——它就是 `pthread_create` 的 C++ 包装，连缺点都原样继承：返回值要靠 `void*` 强转，异常出了线程边界就是进程死刑。

`std::async` 是「任务制」思维：你只声明「**这件事**要并发做」，返回一个 `future` 作为结果的提货单。底层是新线程还是线程池、什么时候调度，由运行时操心。你关心的是任务和结果，不是线程这个硬件资源的账本。

这不是风格之争。三个硬差异——**返回值通道、异常传播、资源管理**——每一个都能在真机上立刻演示出来：thread 里抛异常的进程退出码是 134（SIGABRT），async 里抛异常的进程正常结束。下面全部实测。

---

## 这节讲什么

1. **返回值**：thread 没有任何返回值机制（要自己搭 `promise`），async 直接 `future::get()`；
2. **异常**：thread 里逃逸的异常 → `std::terminate` → 进程 abort（实测退出码 134）；async 的异常穿透到 `get()` 处 rethrow，可正常 try-catch；
3. **资源管理**：thread 是稀缺硬件资源（超订问题），async 把调度权交给运行时；
4. 任务制 vs 线程制心智模型对比表。

---

## 返回值：async 有通道，thread 没有

`std::async` 返回 `std::future<T>`，`get()` 直接拿到任务的返回值。`std::thread` 的 lambda 返回值被直接丢弃——想拿结果得自己搭 `std::promise` 管道，异常还得手动 `set_exception`：

```cpp
#include <future>
#include <iostream>
#include <thread>

int compute() { return 42; }

int main() {
    // async：一行搞定
    auto fut = std::async(std::launch::async, compute);
    std::cout << "async 返回值 = " << fut.get() << "\n";

    // thread：promise/future 管道全手动
    std::promise<int> p;
    auto pfut = p.get_future();
    std::thread t([pp = std::move(p)]() mutable {
        try {
            pp.set_value(compute());
        } catch (...) {
            pp.set_exception(std::current_exception());  // 异常也得手动传
        }
    });
    std::cout << "thread+promise 拿到 = " << pfut.get() << "\n";
    t.join();
    return 0;
}
```

本机实测（clang 23.1.0，`-std=c++17 -Wall`）输出：

```
async 返回值 = 42
thread+promise 拿到 = 42
```

两种写法结果相同，但 thread 版本多出 6 行管道代码，且每一处都是出错点（忘 `set_exception`、promise 提前析构等）。async 把这些全部收进标准库。

---

## 异常：thread 判死刑，async 穿透到 get()

栈展开不跨线程。thread 里没被捕获的异常没有任何人能接住，运行后直接 `std::terminate`：

```cpp
#include <iostream>
#include <stdexcept>
#include <thread>

int main() {
    std::thread t([]{
        throw std::runtime_error("boom from thread");
    });
    t.join();
    std::cout << "这行永远不会执行\n";
    return 0;
}
```

本机实测（clang 23.1.0）stderr 原文与退出码：

```
libc++abi: terminating due to uncaught exception of type std::runtime_error: boom from thread
```

```
exit=134
```

退出码 134 = 128 + 6，即进程被 **SIGABRT** 杀死——不是优雅退出，是堕胎式终止。整个进程陪葬，包括所有无辜线程。

async 版本同一个异常，命运完全不同：

```cpp
#include <future>
#include <iostream>
#include <stdexcept>

int main() {
    auto bad = std::async(std::launch::async, []() -> int {
        throw std::runtime_error("boom from async");
    });
    try {
        (void)bad.get();   // 异常在这里 rethrow，被本线程 catch
    } catch (const std::runtime_error& e) {
        std::cout << "捕获到异常: " << e.what() << "\n";
    }
    std::cout << "进程正常结束\n";
    return 0;
}
```

本机实测（clang 23.1.0）输出，退出码 0：

```
捕获到异常: boom from async
进程正常结束
```

机制：任务抛出的异常被 async 的实现捕获，存进 future 的共享状态；`get()` 时在**调用方线程**重新抛出，走正常的 try-catch 路径。代价是要记得 `get()`——没人 `get()`，异常就被静默丢弃（顺带一提：clang 23 的 libc++ 给 `get()` 标了 `[[nodiscard]]`，忽略它会吃 `-Wunused-result` 警告，实测如此）。

---

## 资源管理：线程是稀缺资源

`std::thread` 直接对应 OS 线程：创建有内核开销，数量有上限，创建失败抛 `std::system_error`。如果并发请求突然暴涨（行情风暴时风控任务激增），手动 `std::thread` 的版本只有两个结局：超订（oversubscription——线程数远超核数，调度抖动摧毁延迟）或直接抛异常拒绝服务。**不要真去试创建十万线程**——文档层面理解即可：线程是硬件资源，像文件描述符一样会被耗尽。

`std::async` 把「任务」和「线程」解耦：你提交任务，运行时可以新开线程、复用线程池、甚至（默认策略下）推迟执行（[Item 36](item36-launch-policy.md) 的陷阱在这里埋下伏笔）。理想实现下，任务制让并发度自动匹配硬件能力。

心智模型对比：

| 维度 | `std::thread`（线程制） | `std::async`（任务制） |
|------|------------------------|------------------------|
| 抽象 | OS 线程的句柄 | 异步任务的提货单（`future`） |
| 返回值 | 无机制，手动搭 `promise` | `future::get()` |
| 异常 | 逃逸即 `std::terminate`（实测 exit 134） | 存进 future，`get()` 时 rethrow |
| 线程管理 | 手动 join/detach，忘 join 析构即 terminate | 运行时托管 |
| 资源控制 | 程序员自行防超订 | 实现可池化/延迟（默认策略下） |
| 适用 | 需要精细控制线程本身（亲和性、栈大小、专用线程） | 绝大多数「并发做件事」的场景 |

---

## 完整可运行例子

async 的两大能力合在一个程序里：

```cpp
#include <future>
#include <iostream>
#include <stdexcept>

int compute() { return 42; }

int main() {
    // 1. 返回值通道
    auto fut = std::async(std::launch::async, compute);
    std::cout << "async 返回值 = " << fut.get() << "\n";

    // 2. 异常穿透：任务里 throw，get() 时 rethrow
    auto bad = std::async(std::launch::async, []() -> int {
        throw std::runtime_error("boom from async");
    });
    try {
        (void)bad.get();
    } catch (const std::runtime_error& e) {
        std::cout << "捕获到异常: " << e.what() << "\n";
    }
    std::cout << "进程正常结束\n";
    return 0;
}
```

本机实测（clang 23.1.0，`-std=c++17 -Wall`）输出：

```
async 返回值 = 42
捕获到异常: boom from async
进程正常结束
```

---

## 常见错误（新手踩坑）

1. **`std::thread` 里让异常逃逸**。实测进程直接被 SIGABRT 杀死（exit 134），`libc++abi: terminating due to uncaught exception`——所有线程陪葬。修正：线程函数体内兜底 try-catch，或改用 `std::async`。
2. **以为 `std::thread` 的 lambda 返回值能拿回来**。返回值被直接丢弃。修正：要结果就用 `std::async`；坚持用 thread 就自己搭 `std::promise` 管道（含 `set_exception`）。
3. **`std::thread` 析构时忘记 join/detach**。joinable 的 thread 析构即 `std::terminate`（Item 37 详讲）。修正：RAII 封装或改用 async。
4. **`async` 返回的 future 从不 `get()`**。任务里的异常被静默丢弃，且默认/强制 async 策略下 future 析构会阻塞到任务完成（Item 38）。修正：结果和异常都要取。

---

## HFT 关联

- **守护任务的异常隔离**：风控预检、日志落盘、快照上传这类后台任务用 `std::async`——任务抛异常穿透到 `get()`，守护进程捕获后继续跑；用裸 `std::thread`，一次未捕获异常就是整个交易进程 SIGABRT（实测 exit 134），盘中事故。
- **热路径仍用专用 thread**：`async` 是「任务制」便利，但延迟关键的行情处理线程需要钉核（CPU affinity）、实时调度、自定义栈——这些是 `std::thread`（或 `pthread`）的领域。规则：**后台任务用 async，热路径专用线程用 thread 并自己管好它**。
- **防超订**：行情风暴触发的瞬时任务洪峰，裸 thread 版本会超订拖垮全系统；async（尤其配合线程池实现）给出自然的并发上限。
- **future 即回执**：`std::async` 返回的 `future` 天然是任务回执——下单网关异步确认、回测任务批量收集结果，`get()`/`wait_for()` 就是同步点。

---

## 自测题

**1. `std::async` 相比 `std::thread` 的三个核心优势是什么？**
<details><summary>答案</summary>
①返回值通道：`async` 返回 `std::future<T>`，`get()` 直接拿结果；`thread` 无任何机制，要自己搭 `std::promise`。②异常安全：任务异常被存进 future，`get()` 时在调用方线程 rethrow，可 try-catch；`thread` 里逃逸的异常直接 `std::terminate`（本文实测进程被 SIGABRT 杀死，exit 134）。③资源管理：`async` 把线程生命周期和调度交给运行时（可池化）；`thread` 要手动 join/detach，析构时 joinable 即 terminate，且线程是稀缺资源，手动管理有超订风险。
</details>

**2. `std::thread` 的线程函数里抛出未捕获异常，会发生什么？退出码是多少？**
<details><summary>答案</summary>
栈展开不跨线程，没有任何人能捕获这个异常，运行时调用 `std::terminate()`，进程被 SIGABRT 杀死。本机实测 stderr 输出 `libc++abi: terminating due to uncaught exception of type std::runtime_error: boom from thread`，退出码 134（= 128 + SIGABRT 的 6）。「这行永远不会执行」——`t.join()` 之后的代码没有机会跑。
</details>

**3. `std::async` 任务的异常如何到达调用方？如果没人调用 `get()` 会怎样？**
<details><summary>答案</summary>
任务抛出的异常被 async 实现捕获并存入 future 的共享状态；调用方 `get()` 时异常在自己的线程里被 rethrow，走正常 try-catch（本文实测捕获到 `boom from async`，进程 exit 0）。如果从不 `get()`，异常被静默丢弃——任务失败无人知晓。另外 libc++ 给 `get()` 标了 `[[nodiscard]]`，忽略返回值会有编译警告。
</details>

**4. 坚持用 `std::thread` 又想拿返回值，怎么办？代价是什么？**
<details><summary>答案</summary>
自己搭 `std::promise<T>`/`std::future<T>` 通道：线程里 `set_value(compute())`，异常路径还要 `set_exception(std::current_exception())`，主线程 `fut.get()`。本文实测该写法输出正确（42），但比 `std::async` 多出约 6 行管道代码，每一处（promise 移动进线程、异常转发、join）都是出错点。本质上你手工重新实现了 async 已经提供的东西。
</details>

**5. 什么是超订（oversubscription）？为什么 `std::async` 能缓解它？**
<details><summary>答案</summary>
超订指并发线程数远超硬件核数：每个 `std::thread` 直接对应 OS 线程，任务洪峰时线程暴涨，上下文切换和缓存抖动摧毁延迟，线程耗尽还会抛 `std::system_error`。`std::async` 把任务与线程解耦——实现可以用线程池让并发度匹配硬件，或（默认策略下）推迟执行；程序员只提交任务，不直接持有线程这个稀缺资源。
</details>

**6. 什么场景反而应该用 `std::thread` 而不是 `std::async`？**
<details><summary>答案</summary>
需要控制**线程本身**而非只是并发执行任务时：钉核（CPU affinity）、实时调度策略、自定义栈大小、线程特定的 TLS 生命周期管理、长期存活的专用线程（如 HFT 热路径的行情处理线程）。`async` 不暴露这些控制面。简记：要「一个听话的线程」用 thread，要「一件并发完成的事」用 async。
</details>

---

## 参考与延伸

- 下一节：[Item 36 启动策略](item36-launch-policy.md)
- 回到：[第 7 章 并发 API](README.md)
