# 3.1 inline 变量的动机与用法

> 第 3 章 内联变量 · 下一节：[3.2 头文件全局对象与 ABI 影响](02-头文件全局对象与ABI影响.md)

## 这节讲什么

C++17 之前，在头文件里定义全局变量会导致 ODR（One Definition Rule）违规——每个包含该头文件的翻译单元都会生成一份定义，链接时报"重复定义"。C++17 的 `inline` 变量解决了这个问题。

## 为什么要学这个（先建立直觉）

C 程序员在头文件定义全局变量的方式：

```c
// config.h
extern int MAX_ORDERS;  // 声明

// config.c
int MAX_ORDERS = 1000;  // 定义（只能在一个 .c 文件里）
```

C++ 同样的模式：

```cpp
// config.h
extern int MAX_ORDERS;  // 声明

// config.cpp
int MAX_ORDERS = 1000;  // 定义
```

问题：每个全局变量要写两遍（头文件声明 + cpp 定义），维护麻烦。

C++17 inline 变量一步到位：

```cpp
// config.h
inline int MAX_ORDERS = 1000;  // 声明 + 定义，头文件直接写
// 包含此头文件的所有 .cpp 共享同一个 MAX_ORDERS
```

## inline 变量的语义

```cpp
inline int counter = 0;       // 全局 inline 变量
inline constexpr int BUF = 1024;  // constexpr 隐含 inline（C++17 起）

struct Config {
    static inline int version = 42;  // 类内 static inline 变量
};
```

### 关键特性

1. **可以定义在头文件中**：多个翻译单元包含同一头文件，只有一个定义（ODR 合规）
2. **链接器保证唯一性**：类似 inline 函数，链接器合并多份定义为一份
3. **初始化保证**：程序启动时初始化（静态初始化）

## 对比 C++14 的痛点

### C++14：类内 static constexpr

```cpp
// C++14
struct Config {
    static constexpr int BUF_SIZE = 1024;  // 类内声明 + 定义（仅对 constexpr）
    static const int TIMEOUT = 5000;       // 类内声明，需要类外定义
};

// C++14 还需要在 .cpp 里类外定义（ODR-use 时）
// const int Config::TIMEOUT;  // 否则链接报 undefined reference
```

### C++17：inline 变量一统天下

```cpp
// C++17
struct Config {
    static inline int BUF_SIZE = 1024;     // 直接定义，不需要类外定义
    static inline int timeout = 5000;      // 非 constexpr 也可以
    static inline std::string name = "HFT";// 复杂类型也行
};
// 不需要任何类外定义！
```

## 使用场景

### 1. 头文件全局常量

```cpp
// constants.h
inline constexpr int MAX_ORDER_BOOK_DEPTH = 10;
inline constexpr double MIN_SPREAD = 0.0001;
inline constexpr const char* EXCHANGE_NAME = "NYSE";
```

### 2. 类静态成员

```cpp
class Exchange {
    static inline int instance_count = 0;  // 类内直接定义
public:
    Exchange() { ++instance_count; }
    ~Exchange() { --instance_count; }
    static int count() { return instance_count; }
};
```

### 3. 头文件中的单例

```cpp
// logger.h
inline Logger& global_logger() {
    static Logger instance;  // 局部 static（C++11 起线程安全）
    return instance;
}
// 或者
inline Logger g_logger;  // C++17 inline 变量
```

### 4. 模板库中的配置

```cpp
// 在 header-only 库中定义全局配置
inline int pool_default_size = 4096;

// 用户可以在 main() 开头修改
int main() {
    pool_default_size = 8192;
    // ...
}
```

## HFT 关联

```cpp
// HFT 全局配置（头文件定义）
struct HFTConfig {
    static inline int ring_buffer_size = 65536;  // SPSC 队列大小
    static inline int batch_size = 16;           // 每批处理订单数
    static inline bool cpu_affinity = true;      // 绑核
    static inline int latency_budget_us = 10;    // 延迟预算
};
```

## 小结

| 特性 | C++14 | C++17 |
|------|-------|-------|
| 头文件全局变量 | extern 声明 + .cpp 定义 | `inline` 一行搞定 |
| 类 static 成员 | 类内声明 + 类外定义 | `static inline` 类内定义 |
| constexpr 变量 | 隐含 inline | 仍然隐含 inline |
| 非 constexpr 类 static | 需要类外定义 | `static inline` 直接定义 |

---

← [本章导读](./README.md) · [下一节 →](02-头文件全局对象与ABI影响.md)
