# 2.2 按位拷贝 vs 逐成员拷贝

> 第 2 章 · 上一节：[2.1 合成默认构造](01-synthesized-default-ctor.md) · 下一节：[2.3 成员初始化列表](03-init-list.md)

## 这节讲什么

拷贝构造默认是按位拷贝（逐字节 memcpy）。只有当类含四种情况之一时，编译器才合成逐成员拷贝。按位拷贝对含指针的类是灾难——浅拷贝导致双重释放。

---

## 为什么要学这个（先建立直觉）

C 程序员对结构体赋值的心智模型是 memcpy：

```c
struct Data_C {
    int values[100];
};
struct Data_C a = {0};
struct Data_C b = a;  // memcpy(&b, &a, sizeof(Data_C)) —— 安全
```

但如果结构体里有指针：

```c
struct Bad_C {
    int* data;     // 指针！
};
struct Bad_C a;
a.data = malloc(100 * sizeof(int));
struct Bad_C b = a;  // memcpy → b.data == a.data（同一个指针！）
free(a.data);
free(b.data);  // double free！b.data 和 a.data 指向同一块内存
```

C 程序员知道这个坑，但 C++ 让这个坑更隐蔽——因为编译器有时帮你合成"正确的"拷贝（逐成员拷贝），有时不帮（按位拷贝）：

```cpp
class Safe {
    std::string name;  // string 有拷贝构造 → 编译器合成逐成员拷贝 → 深拷贝
};
Safe a;
Safe b = a;  // 安全！name 被深拷贝

class Dangerous {
    int* data;         // 原始指针，没有拷贝构造 → 按位拷贝 → 浅拷贝
};
Dangerous a;
a.data = new int[100];
Dangerous b = a;  // 危险！b.data == a.data
```

---

## 两种拷贝机制详解

### 按位拷贝（bitwise copy）

```cpp
struct POD { int x; double y; };
POD a = {42, 3.14};
POD b = a;  // 逐字节 memcpy(&b, &a, sizeof(POD))
// 对 POD 安全——纯数据，没有需要"调用"的东西
```

### 逐成员拷贝（memberwise copy）

```cpp
class WithString {
    std::string name;
    int level;
};
WithString a;
WithString b = a;
// 编译器合成的拷贝构造：
// 1. name.string::string(a.name)  → 调 string 的拷贝构造（深拷贝）
// 2. level(a.level)               → 直接拷贝
```

### 四种触发逐成员拷贝的情况

1. 类含有自定义拷贝构造的成员对象
2. 基类有自定义拷贝构造
3. 类有虚函数（需调整 vptr）
4. 类有虚基类（需调整虚基类指针）

---

## 常见错误（新手踩坑）

### 错误 1：含指针的类不写拷贝构造

```cpp
class Buffer {
    int* data;
    size_t size;
public:
    Buffer(size_t n) : data(new int[n]), size(n) {}
    // 没写拷贝构造 → 按位拷贝 → 浅拷贝
    ~Buffer() { delete[] data; }
};
Buffer a(100);
Buffer b = a;  // b.data == a.data
// a 和 b 析构时都 delete[] 同一块内存 → double free → 崩溃
```

### 错误 2：写了拷贝构造但忘了拷贝所有成员

```cpp
class Config {
    int port;
    std::string host;
public:
    Config(const Config& other) : port(other.port) {
        // 忘了 host！host 会被默认构造（空字符串）
    }
};
```

### 错误 3：以为 vector<T> 拷贝是 memcpy

```cpp
std::vector<int> v1 = {1, 2, 3};
std::vector<int> v2 = v1;  // 不是 memcpy！
// vector 有自定义拷贝构造 → 逐成员拷贝 → 深拷贝内部数组
// 如果 T 是 POD，数组部分用 memcpy；但 vector 本身的拷贝不是 memcpy
```

---

## 和 C 的区别

| 特性 | C struct 赋值 | C++ class 拷贝 |
|------|-------------|----------------|
| 默认行为 | 按位拷贝（memcpy） | POD: 按位拷贝；非 POD: 逐成员拷贝 |
| 指针成员 | 浅拷贝（和 C++ 一样危险） | 浅拷贝（如果没写拷贝构造） |
| 成员对象 | N/A | 自动调成员的拷贝构造 |
| 大三律 | 手动管理 | **需要析构/拷贝/赋值之一就要全部三个** |

**大三律（Rule of Three）**：如果你需要手写析构函数、拷贝构造、拷贝赋值中的任何一个，你就需要写全部三个。C++11 后扩展为大五律（加上移动构造和移动赋值）。

---

## HFT 关联

1. **POD 可安全 memcpy**：POD 类型按位拷贝是安全的——`vector<Tick>` 扩容时 `memcpy` 移动旧元素，零拷贝构造开销。HFT 数据结构尽量 POD。
2. **避免含指针的类**：用 `std::unique_ptr`/`std::shared_ptr` 替代裸指针——智能指针有自定义拷贝/移动语义，自动正确处理。
3. **大五律检查**：HFT 代码审查 checklist——任何类如果有裸指针成员，必须检查大五律是否完整。

---

## 代码自测

### Q1: 按位 vs 逐成员

```cpp
struct A { int x; double y; };               // POD
struct B { std::string s; };                  // 非 POD
A a1 = {1, 2.0}; A a2 = a1;   // 哪种拷贝？
B b1; B b2 = b1;               // 哪种拷贝？
```

<details>
<summary>答案与复习指引</summary>

A 是 POD → 按位拷贝（memcpy）。B 非 POD（string 有自定义拷贝构造）→ 逐成员拷贝（调 string 的拷贝构造，深拷贝）。

**复习：** → [2.2 按位拷贝 vs 逐成员拷贝](./02-bitwise-vs-memberwise.md)
</details>

### Q2: 浅拷贝灾难

```cpp
class Widget {
    int* data;
public:
    Widget() : data(new int[100]) {}
    ~Widget() { delete[] data; }
};
Widget a;
Widget b = a;  // 会发生什么？
```

<details>
<summary>答案与复习指引</summary>

浅拷贝：`b.data == a.data`（指向同一块内存）。当 a 和 b 析构时，`delete[]` 同一块内存两次 → double free → UB（可能崩溃）。修正：写拷贝构造做深拷贝，或用 `std::vector<int>` 替代裸指针。

**复习：** → [2.2 按位拷贝 vs 逐成员拷贝](./02-bitwise-vs-memberwise.md)
</details>

### Q3: 大三律

```cpp
class Resource {
    FILE* fp;
public:
    Resource(const char* path) : fp(fopen(path, "r")) {}
    ~Resource() { if (fp) fclose(fp); }
    // 还缺什么？
};
```

<details>
<summary>答案与复习指引</summary>

缺拷贝构造和拷贝赋值（大三律）。两个 Resource 对象会共享同一个 `fp`，析构时 fclose 两次。修正：要么删除拷贝（`Resource(const Resource&) = delete;`），要么实现深拷贝（重新 fopen）。

**复习：** → [2.2 按位拷贝 vs 逐成员拷贝](./02-bitwise-vs-memberwise.md)
</details>

### Q4: POD 的优势

```cpp
struct Tick {
    int price;
    int qty;
    long timestamp;
};
// vector<Tick> 扩容时如何移动元素？有什么优势？
```

<details>
<summary>答案与复习指引</summary>

Tick 是 POD，`vector<Tick>` 扩容时用 `memcpy`（或 `memmove`）移动元素——零拷贝构造开销。如果是非 POD 类型，需要逐个调拷贝/移动构造，慢得多。HFT 数据结构尽量 POD 以获得 `memcpy` 级别的性能。

**复习：** → [2.2 按位拷贝 vs 逐成员拷贝](./02-bitwise-vs-memberwise.md)
</details>

---

## 参考与延伸

- 下一节：[2.3 成员初始化列表](03-init-list.md)
- 回到：[第 2 章 构造函数语义](README.md)
