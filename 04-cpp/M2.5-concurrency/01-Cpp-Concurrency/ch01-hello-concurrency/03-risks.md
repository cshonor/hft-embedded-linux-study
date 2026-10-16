# 1.3 并发风险

> 第 1 章 · 上一节：[1.2 并发动机](02-motivation.md) · 下一章：[第 2 章 管理线程](../ch02-managing-threads/README.md)

## 这节讲什么

并发引入四大风险：数据竞争、死锁、悬垂引用、线程开销。理解每种风险的发生条件和后果，才能在编码时主动防范。

## 为什么要学这个（先建立直觉）

C 程序员可能觉得"我写多线程没问题"——但 C++ 的并发风险有一个 C 里不存在的放大器：**编译器优化**。

```c
// C 程序员的直觉：这段代码"应该"工作
int flag = 0;
int data = 0;

// 线程 1
data = 42;
flag = 1;

// 线程 2
while (flag != 1) {}
printf("%d\n", data);  // 期望打印 42
```

在 C++ 中，这段代码是**未定义行为**。编译器可能：
1. 把 `flag` 缓存到寄存器，`while(flag!=1)` 变成死循环
2. 重排 `data=42` 和 `flag=1` 的顺序，线程 2 可能看到 `flag=1` 但 `data=0`

```cpp
// C++ 正确写法：用 atomic + 内存序
std::atomic<int> flag{0};
int data = 0;

// 线程 1
data = 42;
flag.store(1, std::memory_order_release);  // release：之前的写对读者可见

// 线程 2
while (flag.load(std::memory_order_acquire) != 1) {}  // acquire：看到 flag=1 后，data 一定可见
std::cout << data;  // 保证打印 42
```

## 四大风险详解

### 风险 1：数据竞争（Data Race）

```cpp
// 数据竞争：多线程无同步访问共享可变数据
int counter = 0;
void worker() {
    for (int i = 0; i < 100000; i++)
        counter++;  // 读-改-写三步，非原子
}
std::thread t1(worker), t2(worker);
t1.join(); t2.join();
// counter 可能不是 200000——丢失更新

// 修复方案 1：atomic
std::atomic<int> counter{0};
counter++;  // 原子操作，安全

// 修复方案 2：mutex
std::mutex mtx;
int counter = 0;
void worker() {
    for (int i = 0; i < 100000; i++) {
        std::lock_guard<std::mutex> lk(mtx);
        counter++;
    }
}
```

**关键**：数据竞争不是"可能出错"，而是**UB**。编译器有权假设数据竞争不会发生，基于此做激进优化。

### 风险 2：死锁（Deadlock）

```cpp
// 经典死锁：交叉锁序
std::mutex m1, m2;

void thread_a() {
    std::lock_guard<std::mutex> lk1(m1);
    // ... 此处被调度切换 ...
    std::lock_guard<std::mutex> lk2(m2);  // 等 m2
}
void thread_b() {
    std::lock_guard<std::mutex> lk1(m2);
    std::lock_guard<std::mutex> lk2(m1);  // 等 m1 → 死锁！
}

// 修复：C++17 scoped_lock 原子锁多个
void safe_thread() {
    std::scoped_lock lk(m1, m2);  // 原子获取，不会死锁
}
```

### 风险 3：悬垂引用（Dangling Reference）

```cpp
// 悬垂引用：线程引用了已销毁的局部变量
void buggy() {
    int local = 42;
    std::thread t([&local] {
        std::cout << local;  // local 可能已销毁！
    });
    t.detach();  // 分离，函数返回后 local 销毁
}  // local 在这里销毁——线程可能还在用它

// 修复：按值捕获或移动
void safe() {
    int local = 42;
    std::thread t([local] {  // 按值拷贝
        std::cout << local;
    });
    t.detach();  // 安全：local 是线程自己的副本
}
```

### 风险 4：线程开销

```cpp
// 线程开销：创建/调度/销毁有成本
// 每个线程：栈内存（默认 1-8MB）+ 内核对象 + 调度开销
for (int i = 0; i < 10000; i++)
    std::thread(small_task).detach();
// 10000 个线程 × 8MB 栈 = 80GB 内存！
// 调度抖动：上下文切换 ~1-10μs，频繁切换导致 cache miss

// 修复：线程池
std::vector<std::thread> pool;
// 启动时创建固定数量线程，任务入队列
```

| 风险 | 后果 | 检测难度 | 修复方案 |
|------|------|----------|----------|
| 数据竞争 | UB（结果不确定、崩溃） | 极难（时序相关） | atomic/mutex |
| 死锁 | 程序卡死 | 中（可复现） | scoped_lock/锁序 |
| 悬垂引用 | UB/段错误 | 难（时序相关） | 按值捕获/移动 |
| 线程开销 | 性能下降/内存耗尽 | 易（可监控） | 线程池 |

## 常见错误（新手踩坑）

### 错误 1：以为 `volatile` 能解决数据竞争

```cpp
// 错误：volatile 不保证原子性，也不保证内存序
volatile int counter = 0;
void worker() { for (int i=0;i<100000;i++) counter++; }  // 仍然数据竞争！

// volatile 只告诉编译器"别缓存到寄存器"
// 但不阻止 CPU 重排，也不保证读-改-写的原子性
```

**修复**：用 `std::atomic<int>`。C++ 的 `volatile` 和 Java 的 `volatile` 不同——C++ 的 `volatile` 只用于硬件寄存器/信号处理，不用于线程同步。

### 错误 2：在析构函数里 join 线程导致死锁

```cpp
class Bad {
    std::thread t;
public:
    ~Bad() {
        if (t.joinable()) t.join();  // 如果 t 正在等 Bad 的某个成员...
    }                                 // → 死锁
};
```

**修复**：确保线程不依赖对象自身的状态，或在线程函数中加退出标志。

### 错误 3：异常路径忘记 join

```cpp
void risky() {
    std::thread t(long_task);
    do_something();  // 抛异常！
    t.join();        // 永远到不了 → t 析构时 terminate
}
```

**修复**：用 RAII 守卫（`joining_thread` 或 C++20 `std::jthread`），或 try/catch + join。

## 和 C 的区别

| 特性 | C | C++ |
|------|---|-----|
| 数据竞争后果 | UB（但编译器优化较保守） | UB（编译器更激进，后果更严重） |
| volatile 语义 | 可能"看起来"能工作 | 明确不用于线程同步 |
| 原子操作 | `__atomic`/`__sync` GCC 扩展 | `std::atomic`（标准化） |
| 线程安全初始化 | pthread_once | `std::call_once` / static 局部变量 |
| 死锁检测 | 手动 | scoped_lock（C++17） |

## HFT 关联

- **数据竞争是 HFT 的头号敌人**：数据竞争导致的非确定延迟无法复现、无法调试——可能在生产环境偶尔出现，测试环境永远复现不了。
- **死锁 = 系统停摆**：HFT 守护进程死锁意味着停止交易，可能导致巨额亏损。所有锁路径必须用 `scoped_lock` 或固定锁序。
- **线程开销 = 延迟抖动**：线程创建/销毁导致内存分配 + 系统调用，产生微秒级抖动。HFT 启动时建固定线程池，永不创建/销毁线程。

## 代码自测

### Q1: 下列代码输出什么？为什么？

```cpp
volatile int x = 0;
std::thread t1([&] { for (int i=0;i<100000;i++) x++; });
std::thread t2([&] { for (int i=0;i<100000;i++) x++; });
t1.join(); t2.join();
std::cout << x;
```

<details>
<summary>答案与复习指引</summary>

**输出不确定**（可能是 100000~200000 之间的任何值）。`volatile` 不保证 `x++` 的原子性——`x++` 是"读-改-写"三步操作，中间可能被中断。

修复：`std::atomic<int> x{0};` 或加锁。

复习：C++ 的 `volatile` ≠ 线程同步。只有 `std::atomic` 才保证原子性和内存序。
</details>

### Q2: 下列代码有什么风险？

```cpp
std::thread make_thread() {
    int local = 42;
    return std::thread([&local] {
        std::this_thread::sleep_for(std::chrono::milliseconds(100));
        std::cout << local << '\n';
    });
}
auto t = make_thread();
t.join();
```

<details>
<summary>答案与复习指引</summary>

**悬垂引用**。`local` 是 `make_thread` 的局部变量，函数返回后销毁。线程通过引用捕获 `local`，sleep 100ms 后访问已销毁的变量——UB。

修复：按值捕获 `[local]` 或用 `std::move`。

复习：引用捕获局部变量 + 线程 detach/延迟执行 = 悬垂引用陷阱。
</details>

### Q3: 下列代码会怎样？

```cpp
void f() {
    std::thread t([] {
        throw std::runtime_error("oops");
    });
    t.join();
}
```

<details>
<summary>答案与复习指引</summary>

**`std::terminate`**。线程函数抛出未捕获异常，`std::thread` 析构会调用 `std::terminate` 拉崩进程。

修复：线程函数内部 try/catch，或用 `std::async`（异常会通过 future 传播）。

复习：线程函数的异常不会自动传播到 join 方——必须在线程函数内捕获。
</details>

### Q4: 为什么 HFT 用线程池而不是按需创建线程？

<details>
<summary>答案与复习指引</summary>

三个原因：
1. **内存**：每个线程栈 1-8MB，频繁创建/销毁导致内存碎片。
2. **延迟**：线程创建涉及系统调用（~10-50μs），HFT 不能容忍。
3. **确定性**：线程创建时机不确定，可能触发调度抖动。固定线程池 + 绑核保证每次执行路径一致。

复习：线程池 = 启动时创建固定线程 + 任务队列 + 线程循环取任务执行。HFT 线程池还绑核 + 设实时优先级。
</details>

---

## 参考与延伸

- 下一章：[第 2 章 管理线程](../ch02-managing-threads/README.md)
- 回到：[第 1 章](README.md)
