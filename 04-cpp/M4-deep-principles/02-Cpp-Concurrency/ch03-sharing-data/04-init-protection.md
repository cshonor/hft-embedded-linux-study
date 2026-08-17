# 3.4 初始化保护

> 第 3 章 · 上一节：[3.3 接口级竞争](03-interface-race.md) · 下一节：[3.5 读写锁](05-shared-mutex.md)

## 这节讲什么

`call_once` 和 Meyers singleton 保证初始化只执行一次且线程安全。C++11 起 `static` 局部变量的初始化由编译器保证线程安全——不再需要手写 DCLP。

## 为什么要学这个（先建立直觉）

C 程序员对"线程安全的单例初始化"很熟悉——经典的做法是 double-checked locking（DCLP）：

```c
// C：DCLP（双重检查锁）——看起来正确但有微妙 bug
static Config* instance = NULL;
static pthread_mutex_t mtx = PTHREAD_MUTEX_INITIALIZER;

Config* get_config() {
    if (instance == NULL) {              // 第一次检查（无锁）
        pthread_mutex_lock(&mtx);
        if (instance == NULL) {          // 第二次检查（有锁）
            instance = new_config();     // 分配 + 构造
        }
        pthread_mutex_unlock(&mtx);
    }
    return instance;
}
```

**问题**：编译器/CPU 可能重排 `new_config()` 内部的"分配内存"和"赋值 instance"——其他线程可能看到 `instance != NULL` 但指向未完全构造的对象。

C++11 之前没有标准的解决方案。C++11 之后，编译器保证 `static` 局部变量的初始化是线程安全的：

```cpp
// C++11：Meyers singleton——编译器保证线程安全
Config& get_config() {
    static Config instance;  // 线程安全初始化（只执行一次）
    return instance;
}
// 无需手写 DCLP，无需手动锁
```

## 两种方式详解

### 方式 1：Meyers Singleton（推荐）

```cpp
class Config {
    int port;
    std::string host;
public:
    Config() : port(8080), host("localhost") {
        // 读配置文件等初始化
    }
    int get_port() const { return port; }
};

Config& get_config() {
    static Config instance;  // C++11 起线程安全
    return instance;
}

// 多线程同时调用 get_config()——只有一个线程执行构造，其他线程等待
```

编译器实现（近似）：

```cpp
// 编译器生成的等价代码
static Config* instance = nullptr;
static std::once_flag flag;

Config& get_config() {
    std::call_once(flag, []{ instance = new Config(); });
    return *instance;
}
```

### 方式 2：std::call_once

```cpp
#include <mutex>

std::once_flag init_flag;
Database* db = nullptr;

void init_database() {
    db = new Database("connection_string");
    db->connect();
}

// 多线程调用，init_database 只执行一次
void ensure_initialized() {
    std::call_once(init_flag, init_database);
}
```

`call_once` 可以接受参数：

```cpp
std::once_flag flag;
void init_with(int port, const std::string& host);

std::call_once(flag, init_with, 8080, "localhost");
```

### 何时用哪个

| 场景 | 推荐 |
|------|------|
| 单例模式 | Meyers singleton（最简） |
| 非函数内的初始化 | `call_once` |
| 需要传参初始化 | `call_once` |
| 类成员延迟初始化 | `call_once` 或 `std::optional` |

## 常见错误（新手踩坑）

### 错误 1：手写 DCLP

```cpp
// 错误：C++11 之前的 DCLP——有内存序 bug
Config* instance = nullptr;
std::mutex m;

Config* get_config() {
    if (!instance) {                    // ① 无锁检查
        std::lock_guard<std::mutex> lk(m);
        if (!instance) {                // ② 有锁检查
            instance = new Config();    // ③ 分配+构造+赋值可能重排！
        }
    }
    return instance;
}
// 其他线程可能在 ③ 完成前看到 instance != NULL → 使用未完全构造的对象
```

**修复**：用 Meyers singleton 或 `call_once`。如果必须手写，用 `std::atomic<Config*>` + `memory_order_acquire/release`。

### 错误 2：函数内 static + 异常

```cpp
Config& get_config() {
    static Config instance;  // 如果构造抛异常呢？
    return instance;
}
// 如果 Config() 抛异常：
// 1. instance 未被初始化
// 2. 下次调用会重新尝试初始化（不是永久失败）
// 3. 但如果异常持续抛出，每次调用都抛——可能影响程序逻辑
```

**注意**：`static` 局部变量初始化抛异常时，下次调用会重试。这通常是期望行为，但要知道这个语义。

### 错误 3：静态成员变量的初始化竞争

```cpp
class Logger {
    static std::ofstream log_file;  // 静态成员——初始化顺序不确定！
public:
    static void log(const std::string& msg) {
        log_file << msg;  // 如果 log_file 还没初始化 → UB
    }
};
// 多个翻译单元的静态成员初始化顺序未定义（"static initialization order fiasco"）
```

**修复**：用 Meyers singleton 包装静态成员：

```cpp
class Logger {
    static std::ofstream& log_file() {
        static std::ofstream f("app.log");  // 按需初始化，线程安全
        return f;
    }
public:
    static void log(const std::string& msg) {
        log_file() << msg;
    }
};
```

## 和 C 的区别

| 特性 | C | C++ |
|------|---|-----|
| 线程安全初始化 | 手写 DCLP（有 bug）或 pthread_once | `static` 局部变量（编译器保证） |
| 一次性调用 | `pthread_once` | `std::call_once` |
| 静态初始化顺序 | 未定义 | 仍有 SIOF（用 Meyers 解决） |
| 内存序保证 | 手动（易错） | 编译器保证 |

## HFT 关联

- **配置/单例**：HFT 守护进程的全局配置/单例用 Meyers singleton，C++11 起保证线程安全初始化。
- **延迟初始化**：HFT 组件（如行情解析器、风控引擎）按需初始化——启动时不初始化所有组件，减少启动时间。
- **避免动态初始化竞争**：HFT 进程启动时多线程并发初始化各组件，用 `call_once` 保证每个组件只初始化一次。

## 代码自测

### Q1: 下列代码线程安全吗？

```cpp
Database& get_db() {
    static Database db("conn_str");
    return db;
}
// 多个线程同时调用 get_db()
```

<details>
<summary>答案与复习指引</summary>

**线程安全**。C++11 起 `static` 局部变量的初始化由编译器保证线程安全——只有一个线程执行 `Database` 构造，其他线程等待。

等价于 `std::call_once` 的语义，但更简洁。

复习：Meyers singleton 是 C++11 起的推荐写法——无需手写 DCLP。
</details>

### Q2: 下列 DCLP 代码有什么问题？

```cpp
Config* ptr = nullptr;
std::mutex m;

Config* get() {
    if (!ptr) {
        std::lock_guard<std::mutex> lk(m);
        if (!ptr) {
            ptr = new Config();  // 这里
        }
    }
    return ptr;
}
```

<details>
<summary>答案与复习指引</summary>

**内存重排 bug**。`new Config()` 分三步：
1. 分配内存
2. 构造对象
3. 赋值给 `ptr`

编译器/CPU 可能重排为 1→3→2——其他线程在 ① 无锁检查时看到 `ptr != NULL`，但对象还没构造完成。

修复（如果必须手写）：
```cpp
std::atomic<Config*> ptr{nullptr};
// 线程 1
Config* p = new Config();
ptr.store(p, std::memory_order_release);
// 其他线程
Config* p = ptr.load(std::memory_order_acquire);
```

或直接用 Meyers singleton。

复习：DCLP 在 C++11 之前是已知 bug——内存序不可控。C++11 后用 `static` 局部变量替代。
</details>

### Q3: 下列代码会怎样？

```cpp
void init() {
    static int count = 0;
    count++;
    std::cout << count << " ";
}
// 3 个线程同时调用 init()
```

<details>
<summary>答案与复习指引</summary>

`count` 的初始化（`= 0`）是线程安全的（只执行一次），但 `count++` **不是线程安全的**——`static` 只保证初始化安全，不保证后续访问安全。

输出可能是 "1 1 1" 或 "1 2 2" 或其他不确定值。

修复：用 `std::atomic<int> count{0};` 或加锁。

复习：`static` 局部变量只保证**初始化**线程安全，不保证**后续访问**线程安全。
</details>

### Q4: 为什么 HFT 用 `call_once` 而不是直接在 `main()` 里初始化？

<details>
<summary>答案与复习指引</summary>

1. **组件化设计**：HFT 各组件（行情、策略、风控等）可能由不同团队开发，各自管理初始化——`call_once` 让组件按需自初始化，不依赖 `main()` 的调用顺序。
2. **延迟初始化**：某些组件（如行情解析器）只在特定条件下才需要——`call_once` 实现真正的 lazy init，减少启动时间。
3. **测试隔离**：单元测试中可能只初始化部分组件——`call_once` 按需初始化，不强制初始化所有组件。

复习：`call_once` 适合组件化、延迟初始化、测试隔离场景。简单程序可以直接在 `main()` 里初始化。
</details>

---

## 参考与延伸

- 下一节：[3.5 读写锁](05-shared-mutex.md)
- 回到：[第 3 章](README.md)
