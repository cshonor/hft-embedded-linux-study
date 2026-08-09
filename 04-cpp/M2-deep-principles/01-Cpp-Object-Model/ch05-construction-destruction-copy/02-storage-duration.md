# 5.2 存储期与生命周期

> 第 5 章 · 上一节：[5.1 构造与析构顺序](01-ctor-dtor-order.md) · 下一节：[5.3 new/delete 的两步](03-new-delete.md)

## 这节讲什么

局部、全局、堆对象的生命周期差异。全局对象的构造在 `main` 前——跨翻译单元的构造顺序未指定（static init order fiasco）。用 Meyers singleton 规避。

---

## 为什么要学这个（先建立直觉）

C 程序员对存储期很熟悉——但 C++ 多了"构造函数"这个维度：

```c
// C：全局变量在 main 前赋值（静态初始化）
int g_count = 42;  // 编译期就确定了值
// C 没有"构造函数"，所以没有初始化顺序问题
```

```cpp
// C++：全局对象的构造函数在 main 前调用
Logger g_logger("app.log");  // 构造函数在 main 前调用
int g_count = g_logger.getLevel();  // 依赖 g_logger 已构造！
// 但 g_count 和 g_logger 在不同文件 → 构造顺序未指定 → 可能 g_logger 还没构造
```

---

## 三种存储期详解

### 局部（栈）存储期

```cpp
void process() {
    Widget w;        // 到达声明点构造
    // ... 使用 w ...
}  // 离开作用域自动析构
// 生命周期：从声明点到作用域结束
```

### 全局/静态存储期

```cpp
// 全局对象
Logger g_logger("app.log");  // main 前构造，main 后析构

// 静态局部
Logger& getLogger() {
    static Logger logger("app.log");  // 首次调用时构造
    return logger;
}  // main 后析构（atexit 注册）

// 静态成员
class Config {
    static Config instance;  // main 前构造
};
```

### 堆存储期

```cpp
Widget* w = new Widget();  // new 时构造
// ... 使用 w ...
delete w;  // delete 时析构
// 不 delete → 内存泄漏
```

### Static Init Order Fiasco

```cpp
// file1.cpp
extern int g_count;
int g_value = g_count + 1;  // 依赖 g_count

// file2.cpp
int g_count = 42;
// 如果 file1.cpp 的 g_value 先构造 → g_count 还是垃圾值！
// 跨翻译单元的构造顺序未指定
```

### Meyers Singleton 解法

```cpp
int& count() {
    static int c = 42;  // 首次调用时初始化
    return c;
    // C++11 起保证线程安全初始化
}
// 无论调用顺序如何，c 都正确初始化
```

---

## 常见错误（新手踩坑）

### 错误 1：全局对象依赖

```cpp
// config.cpp
Config g_config("config.json");  // 从文件读取配置
// logger.cpp
Logger g_logger(g_config.getLogLevel());  // 依赖 g_config
// 如果 g_logger 先构造 → g_config 还没读配置 → logLevel 是垃圾值
// 修正：Logger& logger() { static Logger l(config().getLogLevel()); return l; }
```

### 错误 2：静态局部变量的线程安全

```cpp
// C++03：静态局部变量初始化不是线程安全的
static Logger& get() {
    static Logger instance("app.log");  // C++03：可能两个线程同时构造
    return instance;
}
// C++11 起：保证线程安全初始化（编译器加锁）
// 所以 C++11+ 可以安全使用 Meyers singleton
```

### 错误 3：忘了 delete

```cpp
void process() {
    Widget* w = new Widget();
    if (error) return;  // 忘了 delete w → 泄漏
    delete w;
}
// 修正：用 std::unique_ptr<Widget> w = std::make_unique<Widget>();
// 离开作用域自动 delete
```

---

## 和 C 的区别

| 特性 | C | C++ |
|------|---|-----|
| 全局变量初始化 | 编译期赋值（静态） | main 前调构造函数（运行时） |
| 初始化顺序 | 无构造函数 → 无顺序问题 | **跨翻译单元顺序未指定** |
| 静态局部 | 首次到达时赋值 | 首次到达时构造（C++11 线程安全） |
| 堆 | malloc/free | new/delete（+ 构造/析构） |
| RAII | 手动 init/cleanup | 自动构造/析构 |

---

## HFT 关联

1. **Meyers singleton**：HFT 守护进程的配置/单例用函数内 static，C++11 起保证线程安全初始化。
2. **避免全局对象**：全局对象的构造顺序不可控——用 Meyers singleton 或显式初始化函数替代。
3. **栈优先**：HFT 热路径对象用栈分配（零 malloc），避免堆分配的不确定延迟。

---

## 代码自测

### Q1: 构造时机

```cpp
struct Logger {
    Logger() { cout << "init "; }
    ~Logger() { cout << "cleanup "; }
};
Logger global;  // 全局
void foo() {
    static Logger local_static;  // 静态局部
    cout << "foo ";
}
int main() {
    cout << "main_start ";
    foo();
    foo();
    cout << "main_end ";
    return 0;
}
// 输出顺序？
```

<details>
<summary>答案与复习指引</summary>

`init main_start init foo foo main_end cleanup cleanup`。全局 `global` 在 main 前构造。静态局部 `local_static` 首次调用 foo() 时构造，第二次不重复。析构在 main 后，逆序：local_static 先，global 后。

**复习：** → [5.2 存储期与生命周期](./02-storage-duration.md)
</details>

### Q2: Static init order fiasco

```cpp
// file_a.cpp
extern int g_b;
int g_a = g_b + 1;
// file_b.cpp
int g_b = 42;
// g_a 的值是什么？
```

<details>
<summary>答案与复习指引</summary>

不确定（UB）。如果 file_b.cpp 的 `g_b` 先初始化，`g_a = 43`。如果 file_a.cpp 的 `g_a` 先初始化，`g_b` 是 0（静态初始化），`g_a = 1`。跨翻译单元的初始化顺序未指定。修正：用函数内 static（Meyers singleton）。

**复习：** → [5.2 存储期与生命周期](./02-storage-duration.md)
</details>

### Q3: Meyers singleton

```cpp
Config& getConfig() {
    static Config cfg("config.json");
    return cfg;
}
// 有什么优势？C++11 起有什么保证？
```

<details>
<summary>答案与复习指引</summary>

优势：①避免 static init order fiasco（首次调用时才构造）；②惰性初始化（不用不构造）。C++11 保证：线程安全初始化（编译器加锁，只构造一次）。HFT 守护进程用这种模式管理配置单例。

**复习：** → [5.2 存储期与生命周期](./02-storage-duration.md)
</details>

### Q4: 栈 vs 堆

```cpp
// 方案 A：栈分配
void processA() {
    Widget w;  // 栈上
    w.run();
}  // 自动析构

// 方案 B：堆分配
void processB() {
    Widget* w = new Widget();  // 堆上
    w->run();
    delete w;
}
// HFT 热路径选哪个？为什么？
```

<details>
<summary>答案与复习指引</summary>

方案 A（栈分配）。栈分配零 malloc（无系统调用），缓存友好（连续内存），自动析构（不会泄漏）。堆分配有 malloc 的不确定延迟（可能触发 mmap/brk），可能 cache miss。HFT 热路径优先栈分配。

**复习：** → [5.2 存储期与生命周期](./02-storage-duration.md)
</details>

---

## 参考与延伸

- 下一节：[5.3 new/delete 的两步](03-new-delete.md)
- 回到：[第 5 章](README.md)
