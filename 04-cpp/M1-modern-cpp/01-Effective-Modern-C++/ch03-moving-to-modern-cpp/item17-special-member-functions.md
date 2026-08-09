# Item 17：理解特殊成员函数的生成规则

> 第 3 章 移步现代 C++ · Item 17 · 上一节：[Item 16 const 线程安全](item16-const-thread-safety.md)

## 为什么要学这个（先建立直觉）

C 的结构体只有值拷贝——`memcpy` 语义，编译器不需要生成任何特殊函数：

```c
struct Point { int x; int y; };
struct Point a = {1, 2};
struct Point b = a;    // 逐字节拷贝——编译器自动做，没有"拷贝构造"的概念
// 离开作用域时自动回收内存——没有"析构函数"
```

C++ 的类有构造、析构、拷贝、移动——编译器会自动生成这些"特殊成员函数"。但生成规则很复杂，**声明了其中一个会影响其余的自动生成**。

最常见的坑：你写了析构函数（比如要释放资源），编译器就**不再自动生成移动构造**。结果 `vector` 扩容时退回拷贝——性能莫名变差，但代码能编译能运行，bug 非常隐蔽。

```cpp
class Order {
    int* data;
public:
    ~Order() { delete[] data; }  // 写了析构
    // 编译器不再自动生成移动构造！
    // → vector<Order> 扩容时退回拷贝 → 性能差
};

// 修正：补上移动操作
class OrderFixed {
    int* data;
public:
    ~OrderFixed() { delete[] data; }
    OrderFixed(OrderFixed&& o) noexcept : data(o.data) { o.data = nullptr; }
    OrderFixed& operator=(OrderFixed&& o) noexcept { /* ... */ }
    OrderFixed(const OrderFixed&) = default;
    OrderFixed& operator=(const OrderFixed&) = default;
};
```

---

## 这节讲什么

C++11 的"大三律"扩展为"大五律"：拷贝构造、拷贝赋值、移动构造、移动赋值、析构。生成规则：**声明了任何一个，其余的生成受抑制**。

---

## 大五律与生成规则

C++ 有 6 个特殊成员函数（含默认构造）：

| 特殊成员函数 | C 术语类比 |
|-------------|-----------|
| 默认构造 | （无——C 自动零初始化或未初始化） |
| 拷贝构造 | `struct B = A;`（memcpy 语义） |
| 拷贝赋值 | `B = A;`（memcpy 语义） |
| 移动构造 | （无——C 没有移动语义） |
| 移动赋值 | （无） |
| 析构 | （无——C 靠程序员手动释放） |

### 生成规则表

| 你声明了 | 编译器自动生成的 |
|----------|-----------------|
| 无任何 | 全部 5 个都生成 |
| 析构函数 | 拷贝操作仍生成；**移动操作被抑制** |
| 拷贝构造 | 拷贝赋值被抑制；移动操作被抑制 |
| 拷贝赋值 | 拷贝构造被抑制；移动操作被抑制 |
| 移动构造 | 拷贝操作被抑制；移动赋值被抑制 |
| 移动赋值 | 拷贝操作被抑制；移动构造被抑制 |

**核心规则：** 声明了任何一个 → 其余四个的自动生成都受影响。手写任何一个就应该把五个都写（或 `=default`/`=delete`）。

### 为什么析构会抑制移动？

C++03 时代没有移动语义。写了析构函数说明类管理资源，拷贝操作需要小心——编译器仍生成拷贝（按成员拷贝），但如果类有指针成员，按成员拷贝会导致浅拷贝（double free）。

C++11 引入移动语义后，编译器变得保守：如果你写了析构但没有显式写移动构造，编译器不敢自动生成移动（怕移动不安全），所以抑制移动、退回拷贝。

```cpp
class Widget {
    int* data;
    size_t n;
public:
    Widget(size_t n) : data(new int[n]), n(n) {}
    ~Widget() { delete[] data; }  // 写了析构
    // 编译器生成：
    //   拷贝构造：按成员拷贝 → data 指针被复制 → double free！
    //   移动构造：被抑制！
};
```

---

## 常见错误（新手踩坑）

**错误 1：写了析构忘了补移动构造**
```cpp
class Order {
    char* buf;
public:
    Order() : buf(new char[64]) {}
    ~Order() { delete[] buf; }
    // 没写移动构造 → 被抑制 → vector 扩容退回拷贝
    // 拷贝构造按成员拷贝 → buf 指针被复制 → double free！
};
```
**修正：** 写了析构就把五件套都写（或 `= default`/`= delete`）。

**错误 2：写了拷贝构造忘了写拷贝赋值**
```cpp
class Config {
    std::string* name;
public:
    Config(const Config& o) : name(new std::string(*o.name)) {}
    // 拷贝赋值被抑制！
};
Config a, b;
b = a;  // 用编译器生成的赋值？不——被抑制了，行为未定义
```
**修正：** `Config& operator=(const Config&) = default;` 或手写。

**错误 3：声明了移动构造后拷贝被抑制，但代码依赖拷贝**
```cpp
class Buffer {
public:
    Buffer(Buffer&&) noexcept;
    // 拷贝被抑制！
};
Buffer a;
Buffer b = a;  // 编译失败！拷贝构造被抑制
```
**修正：** 如果需要拷贝，显式 `= default` 或手写。

---

## 新手要点（和 C 的区别）

| 维度 | C 怎么做 | C++ 怎么做 | 为什么 |
|------|---------|-----------|--------|
| 拷贝 | `memcpy`（总是可用） | 拷贝构造/赋值（可能被抑制） | C++ 有资源管理 |
| 移动 | 不存在 | 移动构造/赋值 | C++11 新增 |
| 析构 | 手动 `free` | 自动调用析构函数 | RAII |
| 生成规则 | 不适用 | 写一个影响其余四个 | C++ 的保守策略 |

**一句话总结：** C 程序员记住——C++ 的类有 5 个特殊成员函数，声明任何一个会影响其余的自动生成。最常踩的坑：写了析构 → 移动被抑制 → vector 扩容退回拷贝 → 性能差。解法：写了任何一个就把五个都显式声明。

---

## HFT 关联

- **移动被抑制导致扩容拷贝**：`Order` 写了析构但没写移动构造 → 扩容退回拷贝 → 延迟尖峰。这是 HFT C++ 性能的隐形坑。
- **RAII 资源管理**：HFT 代码大量用 RAII 管理锁、文件描述符、共享内存——写析构函数是常态，必须同时补移动操作。
- **结构体保持 trivial**：协议结构体不写任何特殊成员函数，保持 trivially copyable，可用 `memcpy` 直接操作网络缓冲区。

---

## 自测题

1. 声明了移动构造后，拷贝构造还会自动生成吗？声明了析构呢（C++11）？
2. "大五律"是哪五个特殊成员函数？
3. 为什么写了析构函数后移动构造会被抑制？这有什么性能影响？
4. "写了任何一个就五个都声明"的规则为什么重要？
5. 下面代码有什么问题？
```cpp
class Buffer {
    char* data;
    size_t size;
public:
    Buffer(size_t n) : data(new char[n]), size(n) {}
    ~Buffer() { delete[] data; }
};
std::vector<Buffer> v;
v.push_back(Buffer(64));
```

---

## 参考与延伸

- 下一章：[第 4 章 智能指针](../ch04-smart-pointers/README.md)
- 回到：[第 3 章 移步现代 C++](README.md)
