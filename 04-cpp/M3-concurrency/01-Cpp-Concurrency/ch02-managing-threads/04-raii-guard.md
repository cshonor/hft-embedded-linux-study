# 2.4 RAII 守卫

> 第 2 章 · 上一节：[2.3 转移所有权](03-transferring-ownership.md) · 下一节：[2.5 硬件并发数](05-hardware-concurrency.md)

## 这节讲什么

用 RAII 包装 `std::thread`，保证所有路径（含异常）都安全收尾。C++20 的 `std::jthread` 是标准化版本，带协作式中断。

## 为什么要学这个（先建立直觉）

C 程序员对"忘了 cleanup"很熟悉：

```c
// C：忘了 pthread_join，资源泄漏但不崩
void buggy() {
    pthread_t t;
    pthread_create(&t, NULL, func, NULL);
    if (error) return;  // 忘了 pthread_join → 泄漏
    pthread_join(t, NULL);
}
```

C++ 的 `std::thread` 更严格——忘了 join 就 `terminate`。但每次手动 join 很容易在异常路径遗漏。RAII 守卫解决了这个问题：

```cpp
// 没有 RAII：异常路径容易遗漏
void risky() {
    std::thread t(long_task);
    do_work();  // 抛异常 → t 析构 → terminate
    t.join();
}

// 有 RAII：所有路径自动 join
void safe() {
    std::thread t(long_task);
    joining_thread guard(std::move(t));  // RAII 守卫接管
    do_work();  // 即使抛异常，guard 析构也会 join
    // guard 析构时自动 join
}
```

## RAII 守卫实现详解

### 自制 joining_thread

```cpp
class joining_thread {
    std::thread t;
public:
    joining_thread() noexcept = default;  // 空构造

    template<typename F, typename... Args>
    explicit joining_thread(F&& f, Args&&... args)
        : t(std::forward<F>(f), std::forward<Args>(args)...) {}

    explicit joining_thread(std::thread&& th) noexcept
        : t(std::move(th)) {}

    joining_thread(joining_thread&&) noexcept = default;  // 可移动
    joining_thread& operator=(joining_thread&&) noexcept = default;

    // 禁止拷贝
    joining_thread(const joining_thread&) = delete;
    joining_thread& operator=(const joining_thread&) = delete;

    ~joining_thread() {
        if (t.joinable()) t.join();  // 析构时自动 join
    }

    // 也可以 detach 或手动 join
    void join() { t.join(); }
    void detach() { t.detach(); }
    bool joinable() const noexcept { return t.joinable(); }
};
```

### 使用方式

```cpp
// 方式 1：直接构造
joining_thread t([] { long_task(); });

// 方式 2：从 std::thread 移动
std::thread raw(func);
joining_thread t2(std::move(raw));

// 方式 3：赋值
joining_thread t3;
t3 = joining_thread([] { another_task(); });

// 所有方式：析构时自动 join，异常路径也安全
```

> **方式 3 有前提（2026-11-11 实测）**：`operator=(joining_thread&&) = default` 落到成员上就是
> `std::thread` 的移动赋值，而它的契约是「目标**还 joinable** → 直接 `terminate()`」。
> 所以 `t3 = ...` 只对**空壳**成立；拿一个线程已经在跑的守卫去接新线程 = 进程当场死：
>
> ```
> a 里已有活线程，现在对 a 做移动赋值…
>   线程 1 干活
>   线程 2 干活
> 退出码=134        ← libc++abi: terminating
> ```
>
> 要支持"覆盖已有线程"，移动赋值得手写（先收旧的、再接新的）：
>
> ```cpp
> joining_thread& operator=(joining_thread&& o) noexcept {
>     if (this != &o) {
>         if (t.joinable()) t.join();   // 先收自己的（会阻塞到旧线程结束，这是语义不是 bug）
>         t = std::move(o.t);           // 此刻目标已非 joinable，安全
>     }
>     return *this;
> }
> ```

### C++20 std::jthread

```cpp
// C++20：标准化的 joining_thread + 协作式中断
#include <stop_token>

std::jthread t([](std::stop_token st) {
    while (!st.stop_requested()) {
        do_work();
    }
});

// 请求停止
t.request_stop();
// jthread 析构时：request_stop() + join()
```

`std::jthread` 相比自制守卫多了：
- 析构时先 `request_stop()` 再 `join()`（不只是盲目 join）
- `stop_token` 协作式中断机制
- 可检查 `stop_requested()` 做优雅退出

## 常见错误（新手踩坑）

### 错误 1：守卫拷贝导致双重 join

```cpp
// 如果不 delete 拷贝构造：
class bad_guard {
    std::thread t;
public:
    ~bad_guard() { if (t.joinable()) t.join(); }
    // 没有 delete 拷贝构造！
};

bad_guard g1{std::thread(func)};   // 花括号：别再写成 (std::thread(func))，那是函数声明，见错误 4
bad_guard g2 = g1;  // 拷贝！
// g1 和 g2 析构时都尝试 join 同一线程 → 第二个 join 是 UB
```

**修复**：`delete` 拷贝构造和拷贝赋值。

### 错误 2：守卫析构时 join 导致死锁

```cpp
class Worker {
    std::jthread t;
    std::mutex mtx;
public:
    Worker() : t([this] { run(); }) {}

    void run() {
        while (true) {
            std::lock_guard<std::mutex> lk(mtx);  // 等 mtx
            // ...
        }
    }

    ~Worker() {
        // jthread 析构时 request_stop() + join()
        // 但如果析构时持有 mtx → 线程等 mtx → join 等线程 → 死锁！
    }
};
```

**修复**：析构前手动释放锁，或在线程函数中检查 stop_token。

### 错误 3：jthread 的 stop_token 没检查

```cpp
// 错误：线程不检查 stop_requested，request_stop 无效
std::jthread t([](std::stop_token st) {
    while (true) {  // 没检查 st.stop_requested()
        blocking_read();  // 阻塞在 IO 上，request_stop 无法唤醒
    }
});
```

**修复**：在循环条件检查 `stop_requested()`，或用 `stop_callback` 注册回调来打断阻塞。

### 错误 4：`scoped_thread t(std::thread(func));` —— 一个对象都没造出来

某课程幻灯片的"使用"示例就长这样，把守卫类写成 `scoped_thread`：

```cpp
class scoped_thread {
    std::thread t;
public:
    explicit scoped_thread(std::thread t_) : t(std::move(t_)) {}
    ~scoped_thread() { t.join(); }
};

scoped_thread t(std::thread(func));    // ← 这行不是"用临时 std::thread 构造 t"
```

编译器把它读成**函数声明**：实参位置 `std::thread(func)` 里的 `(func)` 是合法的
**括号化声明符**，整段等价于

```cpp
scoped_thread t(std::thread func);   // t = 形参 std::thread、返回 scoped_thread 的函数
```

——**没有对象、没有线程、也没有 join**，一行注释都不报。

clang 23.1 实测（给构造函数加计数器，正常应为 1）：

```
mvp.cpp:17:20: warning: parentheses were disambiguated as a function declaration [-Wvexing-parse]
   17 |     scoped_thread t(std::thread(func));
      |                    ^~~~~~~~~~~~~~~~~~~
ctor_count = 0（0 = 这行根本没构造对象）
```

**这比 [2.1](01-thread-lifecycle.md) 的 `std::thread t(Functor())` 更隐蔽**：那次括号里只有
类型名，这次括号里**有名字**（`func`），看着完全像实参——恰恰因为有名字，
`std::thread (func)` 才更像一条合法的参数声明。

**修复**：`scoped_thread t{std::thread(func)};`（`{}` 在语法上不可能是函数声明），
或双括号 `t((std::thread(func)))`。

### 错误 5：加了析构函数，这个守卫就"不能移动"了

幻灯片标题写的是"移动语义与所有权"，但上面那个类实测：

```
is_copy_constructible = 0
is_move_constructible = 0
is_move_assignable    = 0
```

构造函数里 `std::move` 只解决了"把 thread 挪进来"，**类本身一动都不能动**。
原因不是 `std::thread` 不可移动（它只移不拷），而是
**声明了析构函数 → 隐式移动构造/移动赋值不再生成**
（生成规则表见 [13.5 五个特殊成员函数](../../../M0-entry-syntax/01-C++Primer/ch13-copy-control/13.5-五个特殊成员函数与可移动性.md) §二）。
后果很实在：工厂函数返回不了它、`std::vector` 塞不进它、`std::move` 也救不回来。

**修复**：`= default` 显式要回移动（拷贝仍 `= delete`）；移动赋值要手写，见上面"方式 3"。

## 守卫能安全工作的两个前提

### 前提 1：空线程不能 join —— 永远先 `joinable()`

`std::thread` 默认构造出来是**空线程**，只占了位置、没有线程在跑：

```cpp
std::thread t1;            // 空壳：joinable() == false
t1.join();                 // 抛 std::system_error（"Invalid argument"）

std::thread t2([] { work(); });  // 构造即启动：joinable() == true
```

`std::thread` **没有两阶段**——构造函数本身就是"创建并运行"（同 `pthread_create`），不存在"实例化好了再 start"这一步。所以 guard 的析构才永远写成 `if (t.joinable()) t.join();`：守卫可能被移动走（`joinable()` 变 false）、可能空构造后再赋值，裸 join 任何空线程都会抛异常。

### 前提 2：成员声明顺序 —— 数据在前，thread 在后

类持有 thread 成员时，构造和析构两头靠**声明顺序**咬合：

```cpp
class Worker {
    int result_ = 0;        // ① 先声明 → 先构造
    std::thread t_;         // ② 后声明 → 后构造（构造时线程起跑）

public:
    Worker() : t_([this] { result_ = 42; }) {}   // 线程马上访问 result_
    ~Worker() { if (t_.joinable()) t_.join(); }  // join 在成员销毁之前
};
```

- **构造**：成员按**声明顺序**初始化（不看初始化列表的书写顺序）。`t_` 构造的那一刻线程就跑了，它立刻可能访问 `result_`——所以数据必须先声明。顺序写反，线程访问的是未构造对象，UB。
- **析构**：析构函数体在成员销毁**之前**执行，成员再按声明**逆序**销毁（`t_` 先、`result_` 后）。join 写在函数体里 = 保证线程死了 `result_` 才销毁，没人再碰它。

一正一反：`join()` 是唯一能把"线程活着"严格包进"数据活着"之内的操作——`detach` 撒手后这层咬合就断了（见 [2.1 线程生命周期](01-thread-lifecycle.md) 的错误 2 与实验 2 双平台实测）。这也是 `jthread` 析构同样选择 request_stop + join 的原因：**用确定性换等待**。

**多线程扩展**：数量运行时决定就装容器——`std::vector<std::thread>`（thread 只能 move，用 `emplace_back` 造进去），析构里循环 join 全部。原则不变：谁拥有线程谁负责了结，跟数量无关。完整形态（任务队列 + 停止标志）见 [9.1 线程池](../ch09-advanced-thread-management/01-thread-pool.md)。

## 实测记录：把一张幻灯片上的 `scoped_thread` 逐条打坏（2026-11-11，macOS arm64 / clang 23.1 / libc++）

起因是课程幻灯片的这一段（本体 + 用法）：

```cpp
class scoped_thread {
    std::thread t;
public:
    explicit scoped_thread(std::thread t_) : t(std::move(t_)) {}
    ~scoped_thread() { t.join(); }      // 析构时自动 join
};

scoped_thread t(std::thread(func));     // 离开作用域自动回收
```

四条断言，全部真跑：

| # | 断言 | 实测结果 |
|---|---|---|
| 1 | 那行"用法"根本没构造对象 | `-Wvexing-parse` 警告 + 构造计数器 `ctor_count = 0` |
| 2 | 非 joinable 时析构无条件 join | `terminate`，退出码 **134** |
| 3 | 类不可拷贝也不可移动（却在讲移动语义） | 三个 trait 全 **0** |
| 4 | 默认移动赋值吃掉正在跑的线程 | 目标 joinable → `terminate`，退出码 **134** |

第 2 条的原文（传入 `std::thread empty;` 后出作用域）：

```
构造完成，马上离开作用域…
libc++abi: terminating due to uncaught exception of type std::__1::system_error:
    thread::join failed: Invalid argument
退出码=134
```

析构函数隐式 `noexcept`，异常出不去 → `std::terminate`。所以"一律
`if (t.joinable()) t.join();`"不是风格偏好，是**不写就会在某个空线程路径上猝死**。

### 三版对照

| 维度 | 幻灯片 `scoped_thread` | 原书 `scoped_thread`（Williams §2.1.3） | 本节 `joining_thread` |
|---|---|---|---|
| 非 joinable 怎么处理 | 析构裸 `join()` → terminate | **构造时**检查，`throw std::logic_error("No thread")` | 析构 `if (joinable())` |
| 空构造 | 无 | 无 | `= default` |
| 直接接可调用体 | 无（必须先造 `std::thread`） | 无 | 变参构造 `template<F, Args...>` |
| 移动构造 | **没有**（被析构连坐） | 拷贝 `= delete`，移动未写 | `= default` |
| 移动赋值 | 无 | 未写 | `= default`（目标 joinable 会 terminate，见"方式 3"） |
| `join()` / `detach()` / `joinable()` | 无 | 无 | 有 |

取舍很清楚：**要"错就早死"用原书版**（问题暴露在创建点）；**要能空构造、能进容器用本节版**；
幻灯片那版两头不靠——既移动不了，又在析构期才炸。

### 修正版（跑通，退出码 0）

```cpp
class scoped_thread {
    std::thread t;
public:
    explicit scoped_thread(std::thread t_) noexcept : t(std::move(t_)) {}
    ~scoped_thread() { if (t.joinable()) t.join(); }

    scoped_thread(const scoped_thread&)            = delete;
    scoped_thread& operator=(const scoped_thread&) = delete;
    scoped_thread(scoped_thread&&) noexcept = default;
    scoped_thread& operator=(scoped_thread&& o) noexcept {
        if (this != &o) {
            if (t.joinable()) t.join();   // 先收自己的
            t = std::move(o.t);
        }
        return *this;
    }
};
```

实测覆盖：花括号正常构造、传入空 `std::thread`（不炸）、工厂返回后移动构造、
`vector::push_back` 移动入容器——1/2/3/4 号线程全部打印并 join，退出码 0。

> 一句话收尾：**C++20 起不要手写这个类**，`std::jthread` 析构就是
> `request_stop() + join()`；留在 C++11/17 才需要这三件套——
> `joinable()` 判断、`noexcept` 移动、拷贝 `= delete`。

## 和 C 的区别

| 特性 | C (pthread) | C++ (RAII 守卫) | C++20 (jthread) |
|------|-------------|------------------|------------------|
| 自动 join | 无（手动） | 析构时 join | 析构时 join |
| 异常安全 | 无 | 有 | 有 |
| 协作式中断 | 无 | 无 | stop_token |
| 析构行为 | — | 盲目 join | request_stop + join |
| 拷贝保护 | 无 | delete | delete |

## HFT 关联

- **守护进程防崩**：HFT 守护进程里 `std::thread` 析构 terminate 会拉崩进程，用 RAII 守卫或 C++20 `jthread` 保证安全。
- **优雅退出**：HFT 关闭时需要通知所有线程停止（保存状态、刷新订单）→ `jthread` 的 `stop_token` 是理想机制。
- **jthread 的局限**：`request_stop()` 只是设置标志，不抢占。如果线程在 `mutex::lock()` 上阻塞，`stop_token` 无法唤醒它——HFT 需要超时锁 + 定期检查。

## 代码自测

### Q1: 下列代码安全吗？

```cpp
class Task {
    std::thread t;
public:
    Task() : t([this] { this->run(); }) {}
    ~Task() { if (t.joinable()) t.join(); }
    void run() { /* ... */ }
};

Task task;
// Task 析构时会发生什么？
```

<details>
<summary>答案与复习指引</summary>

**可能有死锁**。如果 `run()` 访问 `Task` 的成员，而 `~Task()` 在 `run()` 使用成员时开始析构（先析构成员再 join），顺序错误。

修复：确保 join 在成员析构前完成。`~Task()` 中 `t.join()` 在函数体（成员析构前），所以如果 `run()` 只用 Task 的成员（不在析构期间），是安全的。但如果 `run()` 调用虚函数或其他依赖对象完整性的操作，仍有风险。

复习：RAII 守卫的析构顺序——先 join 线程，再析构成员。确保线程不依赖正在析构的对象。
</details>

### Q2: 下列代码有什么问题？

```cpp
void work() {
    joining_thread t1([] { task1(); });
    joining_thread t2([] { task2(); });
    // t2 先析构（LIFO），等 task2 完成
    // t1 后析构，等 task1 完成
}
```

<details>
<summary>答案与复习指引</summary>

**没有功能问题，但有性能问题**。t2 析构时 join task2，期间 task1 可能已完成但无法被 join（要等 t2 先析构）。串行 join 浪费时间。

如果 task1 和 task2 可以并行结束，总时间是 task2 + task1 而非 max(task1, task2)。

修复（如需优化）：先 detach 或用 future，或改变析构顺序。但通常 LIFO join 是可接受的。

复习：RAII 守卫按 LIFO 顺序析构——后构造的先 join。
</details>

### Q3: 下列 jthread 代码能优雅退出吗？

```cpp
std::jthread t([](std::stop_token st) {
    while (!st.stop_requested()) {
        process_queue();  // 可能阻塞 1 秒
    }
});
// 主线程：t = std::jthread{};  // 触发 request_stop + join
```

<details>
<summary>答案与复习指引</summary>

**能退出，但有延迟**。`request_stop()` 设置标志，但 `process_queue()` 如果正在阻塞，要等它返回后才能检查 `stop_requested()`。最坏情况延迟 1 秒。

如果需要立即退出，`process_queue` 内部需要检查 `stop_token` 或用超时 + `stop_callback`。

复习：`stop_token` 是协作式中断——只设置标志，不抢占。线程必须主动检查。
</details>

### Q4: 为什么 HFT 不完全依赖 jthread 的自动 join？

<details>
<summary>答案与复习指引</summary>

1. **盲目 join 可能阻塞**：HFT 关闭时需要超时——如果某线程卡死，join 会永远阻塞。HFT 需要超时检测 + 强制退出机制。
2. **stop_token 不抢占**：如果线程在 `mutex::lock()` 上阻塞，`request_stop` 无法唤醒它。
3. **确定性退出**：HFT 需要按固定顺序停止线程（先停策略→再停行情→最后停 IO），jthread 的自动析构是 LIFO 顺序，不一定匹配。

HFT 通常自建退出流程：发停止信号→等超时→记录未完成的线程→强制退出。

复习：jthread 适合"正常退出"场景。HFT 需要更健壮的异常退出机制。
</details>

### Q5: 下面两行，哪一行真的启动了线程？

```cpp
scoped_thread a(std::thread(func));   // ①
scoped_thread b{std::thread(func)};   // ②
```

<details>
<summary>答案与复习指引</summary>

**只有 ②**。① 是 most vexing parse：`(std::thread(func))` 被读成参数声明
`std::thread func`，`a` 是函数声明——一个对象都不存在，`func` 永远不会跑。

判据不是"括号里有没有名字"：`std::thread(func)` 里的 `(func)` 本身就是合法的括号化声明符，
所以带名字也照样中招。**统一用 `{}`** 才是免疫写法。

复习：错误 4 的实测（`-Wvexing-parse` + `ctor_count = 0`）。
</details>

### Q6: 为什么这个守卫塞不进 `vector`？

```cpp
std::vector<scoped_thread> v;
v.push_back(scoped_thread{std::thread(func)});   // 编译失败
```

<details>
<summary>答案与复习指引</summary>

因为类**不可移动**：`std::thread` 成员让它不可拷贝，而**用户声明的析构函数**让隐式
移动构造/移动赋值都不生成（13.5 §二），于是 `push_back` 的移动重载也没得用。

报错原文（clang 23.1 / libc++）：

```
error: call to implicitly-deleted copy constructor of 'scoped_thread'
    ::new ((void*)__p) _Up(std::forward<_Args>(__args)...);
```

注意它骂的是**拷贝构造**：没有移动构造时，`push_back(T&&)` 退而求其次用拷贝，
而拷贝早被 `std::thread` 成员删掉了——"说 copy 不说 move"是这类报错最常见的迷惑点。

修复：`scoped_thread(scoped_thread&&) noexcept = default;` 显式要回移动，
移动赋值要手写（先 `if (joinable()) join()` 收掉旧的再接管），否则默认版本在
目标还活着时会走 `std::thread::operator=` 的 terminate 分支。

复习：错误 5 + "方式 3 有前提"那段实测（退出码 134）。
</details>

---

## 参考与延伸

- 下一节：[2.5 硬件并发数](05-hardware-concurrency.md)
- 回到：[第 2 章 管理线程](README.md)
