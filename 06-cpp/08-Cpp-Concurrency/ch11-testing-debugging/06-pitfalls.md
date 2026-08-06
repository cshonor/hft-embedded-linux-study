# 11.6 常见陷阱

> 第 11 章 · 上一节：[11.5 定位技巧](05-locating.md) · 下一章：[附录 A C++11 精要](../appendix-a-cpp11-primer/01-rvalue-move.md)

## 这节讲什么

总结并发编程中最常见的陷阱——这些是无数程序员踩过的坑。本节列举 10 大经典陷阱，每个附代码示例和正确写法。

---

## 核心规则（代码+表格）

### 陷阱1：双重检查锁定（DCLP）的错误

```cpp
// 错误：DCLP 无同步
singleton* instance = nullptr;
singleton* get_instance() {
    if (!instance) {  // 无锁读
        std::lock_guard<std::mutex> lk(m);
        if (!instance) instance = new singleton();  // 可能重排
    }
    return instance;
}
// 问题：new 分三步（分配、构造、赋值），编译器可能重排为
//   分配 → 赋值 → 构造 → 其他线程看到非 null 但未构造完的对象

// 正解1：atomic + double-checked
std::atomic<singleton*> instance{nullptr};
singleton* get_instance() {
    auto* p = instance.load(std::memory_order_acquire);
    if (!p) {
        std::lock_guard<std::mutex> lk(m);
        p = instance.load(std::memory_order_relaxed);
        if (!p) {
            p = new singleton();
            instance.store(p, std::memory_order_release);
        }
    }
    return p;
}

// 正解2：Meyers Singleton（C++11 起线程安全）
singleton& get_instance() {
    static singleton inst;  // C++11 保证局部 static 初始化线程安全
    return inst;
}
```

### 陷阱2：`std::thread` 忘记 join

```cpp
// 错误：thread 析构未 join → terminate
void bad() {
    std::thread t(work);
    // 忘记 t.join() → 析构 → std::terminate
}

// 正解：用 jthread（C++20）或 RAII 守卫
void good() {
    std::jthread t(work);  // 析构自动 join
}
```

### 陷阱3：`condition_variable` 虚假唤醒

```cpp
// 错误：不用谓词
cv.wait(lk);  // 可能虚假唤醒，继续执行但条件不满足

// 正解：用谓词
cv.wait(lk, []{ return ready; });  // 自动循环检查
```

### 陷阱4：`shared_ptr` 的读写竞争

```cpp
// 错误：读写同一个 shared_ptr
std::shared_ptr<T> global = std::make_shared<T>();
// 线程1：global = std::make_shared<T>();  // 写
// 线程2：global->do_something();         // 读+用
// → 数据竞争！shared_ptr 的引用计数是原子的，但指针本身不是

// 正解：用 atomic<shared_ptr>（C++20）或锁保护
std::atomic<std::shared_ptr<T>> global;  // C++20
// 或
std::mutex m;
std::shared_ptr<T> global;
// 读写都加锁
```

### 陷阱5：`this` 指针在成员函数中悬空

```cpp
// 错误：对象可能在 lambda 执行时被销毁
class Bad {
    void start() {
        std::thread t([this]{ this->work(); });  // this 可能悬空
        t.detach();
    }
};
// 如果 Bad 对象在 t 执行前被销毁 → use-after-free

// 正解：用 shared_ptr 延长生命周期
class Good : public std::enable_shared_from_this<Good> {
    void start() {
        auto self = shared_from_this();
        std::thread t([self]{ self->work(); });  // 持有 shared_ptr
        t.detach();
    }
};
```

### 陷阱6：锁内调用虚拟函数

```cpp
// 危险：虚函数可能调用回本对象 → 重入死锁
class Bad {
    std::mutex m;
    virtual void process() {
        std::lock_guard<std::mutex> lk(m);
        do_work();  // do_work 可能调 process() → 重入 lock → 死锁
    }
};
```

### 陷阱7：`volatile` 当原子用

```cpp
// 错误：volatile 不保证原子性和可见性
volatile bool ready = false;
std::thread t1([&]{ ready = true; });
std::thread t2([&]{ while (!ready) {} });  // 可能永远循环

// 正解：用 atomic
std::atomic<bool> ready{false};
std::thread t1([&]{ ready.store(true); });
std::thread t2([&]{ while (!ready.load()) {} });
```

### 陷阱8：移动 `std::thread`

```cpp
// 注意：thread 可移动不可拷贝
std::thread t1(work);
std::thread t2 = t1;        // 编译错误：不可拷贝
std::thread t3 = std::move(t1);  // OK：t1 变为 not-a-thread
// t1.join() → UB（t1 已无关联线程）
```

### 陷阱9：`async` 默认策略的延迟执行

```cpp
// 隐患：默认策略可能延迟到 get() 才执行
auto fut = std::async([]{ return slow_work(); });
// ... 程序继续 ...
int v = fut.get();  // 这里才开始执行 slow_work

// 正解：明确指定 async 策略
auto fut = std::async(std::launch::async, []{ return slow_work(); });
// 立即在新线程执行
```

### 陷阱10：全局变量的初始化竞争

```cpp
// 跨翻译单元的全局变量初始化顺序未定义
// file1.cpp: Logger logger;
// file2.cpp: Config config;  // 如果 config 构造依赖 logger → 顺序不定

// 正解：用 Meyers Singleton（函数内 static）
Logger& get_logger() {
    static Logger logger;
    return logger;
}
// C++11 保证线程安全 + 按需初始化
```

### 十大陷阱速查表

| # | 陷阱 | 正解 |
|---|------|------|
| 1 | DCLP 无同步 | `atomic` 或 Meyers Singleton |
| 2 | thread 忘记 join | `jthread` 或 RAII |
| 3 | cv 虚假唤醒 | 用谓词 `wait(lk, pred)` |
| 4 | shared_ptr 读写竞争 | `atomic<shared_ptr>` 或锁 |
| 5 | this 悬空 | `shared_from_this` |
| 6 | 锁内调虚函数 | 避免或用可重入锁 |
| 7 | volatile 当原子 | `std::atomic` |
| 8 | thread 拷贝 | `std::move` |
| 9 | async 默认延迟 | `launch::async` |
| 10 | 全局初始化竞争 | Meyers Singleton |

---

## 新手要点（和 C 的区别）

- **DCLP 是 C/C++ 共同的经典陷阱**：C 程序员如果用过 DCLP，可能用的是 `pthread_once` 或错误的双重检查。C++11 的 Meyers Singleton 是最简洁的正解——C 程序员要改掉手写 DCLP 的习惯。
- **`volatile` 陷阱是 C 程序员最常犯的**：C 嵌入式编程中 `volatile` 被误用为线程同步——这在 C++ 中同样错误。`std::atomic`（C++）或 `_Atomic`（C11）才是正解。
- **`shared_ptr` 的读写竞争是 C++ 特有**：C 没有 `shared_ptr`，所以没有这个陷阱。C++ 程序员要记住：`shared_ptr` 的引用计数是原子的，但指针本身不是——跨线程读写要用 `atomic<shared_ptr>`。
- **`this` 悬空是 C++ 对象生命周期问题**：C 没有对象析构的概念。C++ 的 `detach` 线程如果捕获 `this`，对象可能在线程执行前析构——`shared_from_this` 是 C++ 特有的解法。

---

## HFT 关联

- **HFT 系统要避免所有 10 个陷阱**：每个陷阱都可能导致崩溃或数据错误——在金融场景下是不可接受的。Code Review 时要专门检查这些模式。
- **DCLP 在 HFT 中用 Meyers Singleton**：HFT 的全局配置、日志器用 Meyers Singleton——C++11 保证线程安全初始化，无需手写 DCLP。
- **`shared_ptr` 读写竞争在 HFT 中常见**：HFT 系统的"策略热切换"——主线程替换策略对象，工作线程读取策略指针。必须用 `atomic<shared_ptr>` 或 RCU（Read-Copy-Update）模式。
- **`this` 悬空在 HFT 回调中常见**：HFT 的回调（如定时器、网络回调）如果捕获 `this`，对象可能已析构。用 `shared_from_this` 或弱引用（`weak_ptr`）。
- **静态分析工具**：HFT CI 流水线应配置 Clang 静态分析 + 自定义 lint 规则——自动检测这些陷阱模式。

---

## 自测题

1. 为什么手写的双重检查锁定（DCLP）是错误的？`new` 的哪三步可能被重排？
2. C++11 的 Meyers Singleton 为什么是线程安全的？
3. `condition_variable::wait(lk, pred)` 的谓词形式解决了什么问题？
4. 为什么跨线程读写同一个 `shared_ptr` 是数据竞争？引用计数不是原子的吗？
5. `detach` 的线程捕获 `this` 有什么风险？如何用 `shared_from_this` 解决？

---

## 参考与延伸

- 下一章：[附录 A.1 右值引用与移动语义](../appendix-a-cpp11-primer/01-rvalue-move.md)
- 上一节：[11.5 定位技巧](05-locating.md)
- 回到：[第 11 章](README.md)
