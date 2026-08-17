# 3.2 头文件全局对象与 ABI 影响

> 第 3 章 内联变量 · 上一节：[3.1 inline 变量的动机与用法](01-inline变量动机与用法.md)

## 这节讲什么

inline 变量虽然方便，但在头文件中定义非平凡对象（如 `std::string`、`std::mutex`）会引入静态初始化顺序问题和 ABI 影响。本节讲清楚什么时候该用、什么时候不该用。

## 静态初始化顺序问题（SIOF）

### 问题场景

```cpp
// a.h
inline std::string g_config = load_config();  // 依赖读文件

// b.h
inline int g_value = parse(g_config);  // 依赖 g_config
```

如果 `g_value` 的初始化在 `g_config` 之前执行，`g_config` 还没构造好，`parse(g_config)` 就是 UB。

**根因**：跨翻译单元的非平凡全局对象，初始化顺序未定义。

### 解决方案：函数内 static（Meyers Singleton）

```cpp
// 安全：局部 static 在首次调用时初始化，C++11 起线程安全
inline std::string& config() {
    static std::string c = load_config();  // 首次调用时初始化
    return c;
}

inline int& value() {
    static int v = parse(config());  // 保证 config() 先初始化
    return v;
}
```

**规则**：非平凡对象（需要运行时初始化）用函数内 static，不用 inline 变量。平凡常量（编译期可计算）用 inline constexpr。

### 什么时候 inline 变量是安全的

```cpp
// 安全：编译期常量
inline constexpr int MAX_SIZE = 1024;
inline constexpr double PI = 3.14159;

// 安全：零初始化
inline int g_counter = 0;
inline bool g_running = false;

// 安全：trivial 类型 + 常量初始化
inline int* g_null = nullptr;

// 不安全：需要运行时初始化
inline std::string g_name = "HFT";        // string 构造需要运行时
inline std::mutex g_mutex;                  // mutex 构造需要运行时
inline std::vector<int> g_data = {1,2,3};  // vector 构造需要运行时
```

## ABI 影响

### 内联函数的 ODR 合并

inline 变量和 inline 函数一样，链接器会合并多份定义。但合并方式取决于平台：

- **ELF**（Linux）：使用 COMDAT 组，链接器保留一份
- **PE/COFF**（Windows）：类似 COMDAT 折叠
- **Mach-O**（macOS）：weak symbol

### 动态库边界

```cpp
// lib.h（被 .so 和主程序同时包含）
inline int g_count = 0;

// 主程序和 .so 各看到一份 g_count 吗？
// 答案：取决于平台和链接方式
// - Linux 默认：主程序和 .so 共享一份（符号介入）
// - Windows DLL：各有一份（DLL 边界隔离）
```

**HFT 注意**：跨 .so 共享 inline 变量时，确保编译器/链接器行为一致。跨平台代码应避免在头文件里用 inline 变量做跨模块共享状态。

## 类静态成员的 inline

### C++14 的老写法

```cpp
// header
class Counter {
    static int count_;  // 声明
};

// .cpp
int Counter::count_ = 0;  // 定义（必须在一个 .cpp 里）
```

### C++17 inline 新写法

```cpp
// header-only
class Counter {
    static inline int count_ = 0;  // 声明 + 定义一步到位
};
// 不需要 .cpp 定义
```

### header-only 库的福音

```cpp
// 一个完整的 header-only 组件
class ThreadPool {
    static inline std::atomic<int> active_count{0};
    static inline thread_local int worker_id = -1;  // C++17 thread_local inline
public:
    static int active() { return active_count.load(); }
};
```

## 常见陷阱

### 陷阱 1：inline 变量被多份初始化

```cpp
// config.h
inline int g_init = expensive_init();

// 如果链接器没有正确合并（某些旧链接器），g_init 可能被初始化多次
// 现代链接器（GCC 7+/Clang 5+/MSVC 2017+）没有这个问题
```

### 陷阱 2：inline constexpr vs inline const

```cpp
inline constexpr int A = 42;  // 编译期常量，可以用于模板参数
inline const int B = 42;      // 运行时常量，不能用于模板参数
// 两者都是 inline 变量，但 constexpr 要求编译期可计算
```

### 陷阱 3：在 inline 变量初始化中抛异常

```cpp
inline std::string g = load_from_file();  // 如果文件不存在 → 抛异常
// 静态初始化期间抛异常 → std::terminate
// 全局对象构造期间抛异常 = 程序直接挂
```

## 最佳实践

| 对象类型 | 推荐方式 | 原因 |
|---------|---------|------|
| 编译期常量 | `inline constexpr` | 零运行时开销 |
| 零初始化 trivial | `inline int x = 0` | 安全 |
| 需运行时初始化 | 函数内 static | 避免 SIOF |
| 跨 .so 共享状态 | 避免 inline 变量 | ABI 不可移植 |
| 类 static 成员 | `static inline` | header-only 友好 |

## HFT 关联

```cpp
// HFT 全局配置（安全写法）
struct Config {
    // 编译期常量：inline constexpr
    static inline constexpr int CACHELINE = 64;
    static inline constexpr int RING_SIZE = 65536;

    // 运行时配置：函数内 static
    static std::string& exchange() {
        static std::string s = read_env("EXCHANGE");
        return s;
    }

    // 计数器：inline atomic
    static inline std::atomic<uint64_t> order_count{0};
};
```

---

← [上一节](01-inline变量动机与用法.md) · [本章导读](./README.md)
