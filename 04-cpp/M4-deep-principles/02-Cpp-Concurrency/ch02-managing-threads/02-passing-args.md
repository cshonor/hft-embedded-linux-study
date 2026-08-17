# 2.2 传参

> 第 2 章 · 上一节：[2.1 thread 生命周期](01-thread-lifecycle.md) · 下一节：[2.3 转移所有权](03-transferring-ownership.md)

## 这节讲什么

`std::thread` 默认按值拷贝传参。要传引用用 `std::ref`；传指针要注意生命周期；传移动语义用 `std::move`。传参方式直接决定了线程安全性。

## 为什么要学这个（先建立直觉）

C 程序员习惯用 `void*` 给线程传参：

```c
// C：pthread 传参——void* 万能但危险
void* worker(void* arg) {
    int* p = (int*)arg;  // 手动转型，类型不安全
    *p = 42;
    return NULL;
}

int data = 0;
pthread_create(&t, NULL, worker, &data);  // 传指针
// 问题：如果 data 是局部变量，函数返回后悬垂
```

C++ 的 `std::thread` 用模板推导，类型安全，但有一个关键陷阱——**默认按值拷贝，不是引用**：

```cpp
// C++：默认按值拷贝
void increment(int& x) { x++; }

int counter = 0;
std::thread t(increment, counter);  // 编译错误！
// counter 被拷贝，increment 的引用参数无法绑定到拷贝的临时值

// 正确：用 std::ref 传引用
std::thread t(increment, std::ref(counter));  // OK
```

## 传参方式详解

### 方式 1：按值拷贝（默认）

```cpp
void func(int x, std::string s);

std::string s = "hello";
std::thread t(func, 42, s);  // s 被拷贝到线程内部存储
// 线程内部的 s 是副本，不影响外部的 s
```

### 方式 2：传引用（std::ref / std::cref）

```cpp
void modify(std::vector<int>& v) {
    v.push_back(42);
}

std::vector<int> data;
std::thread t(modify, std::ref(data));  // 传引用
t.join();
// data 现在包含 42

// 传 const 引用
void read(const std::string& s);
std::thread t2(read, std::cref(str));
```

**关键**：`std::ref` 返回一个 `std::reference_wrapper`，线程内部解包成引用。不写 `std::ref` 则按值拷贝。

### 方式 3：传指针

```cpp
void worker(int* data, size_t n) {
    for (size_t i = 0; i < n; i++)
        data[i] *= 2;
}

int arr[100];
std::thread t(worker, arr, 100);  // 数组退化为指针
// 注意：arr 必须在线程执行期间有效
```

### 方式 4：传移动语义

```cpp
void consumer(std::unique_ptr<BigData> ptr);

auto ptr = std::make_unique<BigData>();
std::thread t(consumer, std::move(ptr));  // 移动进线程
// ptr 此后为 nullptr
```

| 传参方式 | 语法 | 何时用 | 风险 |
|----------|------|--------|------|
| 按值拷贝 | `t(func, val)` | 小对象、不需要修改原值 | 大对象拷贝开销 |
| 传引用 | `t(func, std::ref(val))` | 需要修改原值 | 悬垂引用（原值销毁） |
| 传 const 引用 | `t(func, std::cref(val))` | 只读大对象 | 悬垂引用 |
| 传指针 | `t(func, ptr)` | 数组、C 风格接口 | 悬垂指针 |
| 传移动 | `t(func, std::move(obj))` | 独占所有权对象 | 无（所有权转移） |

## 常见错误（新手踩坑）

### 错误 1：引用捕获局部变量 + detach

```cpp
void buggy() {
    std::string local = "hello";
    std::thread t([&local] {
        std::this_thread::sleep_for(std::chrono::milliseconds(100));
        std::cout << local;  // local 可能已销毁！
    });
    t.detach();  // 分离
}  // local 在这里销毁
```

**修复**：按值捕获 `[local]` 或用 `std::move(local)`。

### 错误 2：成员函数传参忘记 this

```cpp
class Worker {
    int data;
public:
    void process(int x) { data = x; }
    void start() {
        std::thread t(process, 42);  // 错误！缺少 this
        // 编译错误：process 是成员函数，需要对象
    }
};

// 修复：
std::thread t(&Worker::process, this, 42);  // 传 this 指针
```

### 错误 3：传临时对象的引用

```cpp
void func(const std::string& s);

// 危险：临时 string 的引用
std::thread t(func, std::string("hello"));  // 实际安全——线程会拷贝
// 但如果 func 接受 string&&，行为不同
// 最安全：先创建变量再传
std::string s = "hello";
std::thread t2(func, std::cref(s));  // 明确传引用
```

## 和 C 的区别

| 特性 | C (pthread) | C++ (std::thread) |
|------|-------------|-------------------|
| 传参方式 | `void*`（单一） | 模板推导（多方式） |
| 类型安全 | 无（手动 cast） | 编译期检查 |
| 传引用 | 通过指针间接 | `std::ref` 显式 |
| 传移动 | 不支持 | `std::move` |
| 传成员函数 | 手动封装 | `&Class::method, &obj` |
| 参数拷贝 | 手动 malloc + memcpy | 自动（decay + copy） |

## HFT 关联

- **移动语义避免拷贝**：HFT 传大块行情数据给策略线程时用 `std::move`，避免深拷贝的微秒级开销。
- **指针传递 + 生命周期管理**：HFT 常用预分配的环形缓冲区，线程间传指针 + 序列号，避免拷贝但要确保缓冲区不回绕覆盖。
- **不用引用传参给 detached 线程**：HFT 线程都是显式管理（不 detach），但仍需注意 join 前原变量不能销毁。

## 代码自测

### Q1: 下列代码能编译吗？为什么？

```cpp
void increment(int& x) { x++; }

int counter = 0;
std::thread t(increment, counter);
t.join();
std::cout << counter;
```

<details>
<summary>答案与复习指引</summary>

**编译错误**（或运行时错误，取决于编译器）。`std::thread` 默认按值拷贝参数，`counter` 被拷贝成一个临时值，`int&` 不能绑定到临时值（非 const 引用）。

修复：`std::thread t(increment, std::ref(counter));`

复习：`std::thread` 默认按值拷贝所有参数。要传引用必须用 `std::ref`/`std::cref`。
</details>

### Q2: 下列代码有什么问题？

```cpp
class Manager {
    std::vector<int> data;
public:
    void start() {
        std::thread t(&Manager::process, this);
        t.detach();
    }
    void process() {
        for (auto v : data) std::cout << v;
    }
    ~Manager() { /* data 析构 */ }
};

Manager* m = new Manager();
m->start();
delete m;  // m 析构，data 销毁
// 线程可能还在跑 process()！
```

<details>
<summary>答案与复习指引</summary>

**悬垂 this 指针**。`detach` 后线程持有 `this` 指针，`delete m` 后对象析构，线程访问 `data` 是 UB。

修复：不用 detach——在 `~Manager()` 中 join 线程；或用 `std::shared_ptr<Manager>` 延长生命周期。

复习：传 `this` 给线程 + detach 是经典 bug。对象生命周期必须覆盖线程执行期。
</details>

### Q3: 下列代码输出什么？

```cpp
std::string s = "hello";
std::thread t([](std::string s) {
    std::cout << s;
}, s);
s[0] = 'X';
t.join();
std::cout << s;
```

<details>
<summary>答案与复习指引</summary>

输出 `helloXello`（或 `Xellohello`，取决于线程调度）。

线程内部 `s` 是按值拷贝的副本，打印 "hello"。主线程修改 `s[0]='X'`，join 后打印 "Xello"。

关键：`std::thread` 默认按值拷贝参数，线程内部的 `s` 与外部的 `s` 独立。

复习：按值拷贝意味着线程有自己的副本，互不影响。要共享才用 `std::ref`。
</details>

### Q4: 为什么 HFT 传数据用 `std::move` 而不是引用？

<details>
<summary>答案与复习指引</summary>

1. **避免拷贝开销**：行情数据可能几 KB，深拷贝耗时。`std::move` 只转移指针（~1ns）。
2. **避免悬垂引用**：传引用需要确保原对象在线程执行期间有效，容易出错。移动后所有权归线程，不存在悬垂问题。
3. **明确所有权**：移动语义明确表示"我给你了，你负责"。引用语义模糊——"你能用但我也还能改"。

HFT 典型模式：行情线程把数据 `std::move` 到无锁队列，策略线程取出处理。

复习：移动 > 引用 > 拷贝。移动既高效又安全（所有权明确）。
</details>

---

## 参考与延伸

- 下一节：[2.3 转移所有权](03-transferring-ownership.md)
- 回到：[第 2 章](README.md)
