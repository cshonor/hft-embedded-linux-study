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

#### 实测：不写 std::ref 会怎样（2026-10-16 双平台）

把参数改成引用却直接传，编译器**直接拒绝**，不是警告：

```cpp
void hello(std::string& str) { str += "!"; }
std::string s = "t1";
std::thread t1(hello, s);   // 少了 std::ref
```

Linux（g++ 14.2 / libstdc++）实测报错：

```
error: static assertion failed:
       std::thread arguments must be invocable after conversion to rvalues
```

macOS（clang 23.1 / libc++）实测报错：

```
static_assert failed due to requirement
'__is_invocable_v<void (*)(std::string &), std::string>'
```

**GCC 这句报错把原理直接说出来了**：参数会先「转换成右值」再调用，
`std::string` 的右值绑不到 `std::string&` 上，所以不可调用。
这正是 decay-copy 的含义——去掉引用和 cv 限定，按值拷一份进线程内部存储。

加上 `std::ref(s)` 后两台机器都编译通过，实测 `s` 从 `"t1"` 变成 `"t1!"`，证明引用真的传进去了（不是副本）。

#### ⚠ 修正一个流行说法：「裸传引用会编译报错」只对**非 const** 引用成立

上面的实测用的是 `std::string&`（非 const），编译器确实报错挡下了。但形参是 `const T&` 时**能编译、能跑，传的是拷贝**——没有任何报错：

```cpp
void log(const std::string& msg);

std::string s = "tick";
std::thread t(log, s);   // 编译通过！但传的是副本，不是 s 的引用
// 你以为在共享，实际各改各的——静默坑，编译器不会提醒你
```

原因：decay-copy 出的副本是右值，`const T&` 本来就能绑右值，所以 `is_invocable` 检查通过。

**经验法则**：想让线程函数看到原对象（不管 const 不 const），就写 `std::ref` / `std::cref`；不写就一定是拷贝，别指望编译器报错提醒。

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

### 错误 4：const char* 局部缓冲区——隐式转换发生在新线程（CCiA 经典坑）

```cpp
void f(int i, const std::string& s);

void oops() {
    char buf[64];
    snprintf(buf, sizeof buf, "%d", 42);
    std::thread t(f, 3, buf);   // 拷贝进内部存储的只是 char* 指针！
    t.detach();
}   // buf 在这里销毁 —— 新线程「此刻」才执行 const char* → std::string 转换 → 悬垂，UB
```

机制拆解：`"hello"` 这类实参的类型是 `const char*`，**被 decay-copy 的是指针本身**；
直到新线程里真正 `invoke` 时，才由这个指针构造临时 `std::string` 绑定到 `const std::string&`。
传字符串字面量之所以安全，纯粹因为字面量是静态生存期；换局部 buffer 就炸。

**修复：在构造函数处就完成转换**——转换发生在主线程，拷贝过去的是完整 string：

```cpp
std::thread t(f, 3, std::string(buf));  // 安全
```

#### 实测：ASan 确定性抓到悬垂（2026-11-06，macOS arm64 / clang 23.1 / libc++）

把转换时机放大（`f` 内 sleep 100ms 后再用指针，等价放大「新线程才转换」的窗口），
`-fsanitize=address -g -O1` + `ASAN_OPTIONS=detect_stack_use_after_return=1`：

```
ERROR: AddressSanitizer: stack-use-after-scope
READ of size 3 at 0x... thread T1
    #2 f(int, char const*)                    demo.cpp:12
    #3 std::__1::__invoke<...int&&, char*&&>  invoke.h:90
    #4 std::__1::__thread_execute<...>        thread.h:170
    #5 std::__1::__thread_proxy<...>          thread.h:178
Address is located in stack of thread T0 ... in frame main
    [64, 128) 'buf' (line 16) <== Memory access is inside this variable
```

两个看点：①回溯里的 `__thread_proxy → __thread_execute → __invoke` 正是
「参数先进内部存储、新线程再从存储 invoke」的 libc++ 实现路径，存储里那个 `char*` 就是罪魁；
②ASan 直接标出访问落在 T0 main 栈帧的 `buf` 上——跨线程栈悬垂，肉眼 review 很难稳抓，
ASan 一开就是确定性证据（对应「先让它对（ASan/TSan），再让它快（perf）」的调试顺序）。

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

### Q5: 下面两段代码，哪段有悬垂风险？

```cpp
void log(const std::string& msg);

void a() {
    std::string msg = "tick";
    std::thread t(log, msg);            // A：裸传
    t.detach();
}

void b() {
    std::string msg = "tick";
    std::thread t(log, std::cref(msg)); // B：cref
    t.detach();
}
```

<details>
<summary>答案与复习指引</summary>

**A 安全，B 悬垂**——和直觉相反。

A：拷贝发生在构造函数里、`msg` 销毁前，新线程用的是副本，detach 后与原变量无关。
「默认拷贝」在这里反而是保护伞。

B：`std::cref` 存的是真引用，函数返回后 `msg` 销毁，detach 的线程随时可能读到死对象——
和错误 4 的 `char*` 是同一类 bug。

**教训：detach + ref/cref 是危险组合**，除非能证明原变量活得比线程久（全局变量、
或外层有 join 保证）。两个判据打架时（要共享但活得不够久）是设计问题，
得改生命周期，不是靠 `std::ref` 能糊过去的。

复习：判据①要不要共享原对象？要 → ref/cref；判据②原对象能不能活过线程？不能 → 拷贝或 join。
</details>

### Q6: 怎么把 `std::unique_ptr<Packet>` 传给线程？能用 `std::ref` 吗？

```cpp
void consume(std::unique_ptr<Packet> p);
auto pkt = std::make_unique<Packet>();
```

<details>
<summary>答案与复习指引</summary>

```cpp
std::thread t(consume, std::move(pkt));  // 正确
// 此后 pkt 为空，所有权彻底移交新线程
```

move-only 类型不能拷贝，thread 构造内部用 move 语义接管。

**不要用 `std::ref` 传 unique_ptr**：所有权语义会乱——两个线程都能解引用同一个
`unique_ptr`，析构时双重释放。

复习：独占所有权对象一律 `std::move`；`std::ref` 是给「共享但不转移所有权」的场景用的。
</details>

---

## 参考与延伸

- 下一节：[2.3 转移所有权](03-transferring-ownership.md)
- 回到：[第 2 章](README.md)
