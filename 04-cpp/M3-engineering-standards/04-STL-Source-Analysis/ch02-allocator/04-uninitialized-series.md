# 2.4 uninitialized_* 系列

> 第 2 章 空间配置器 · 第 4 节 · 上一节：[2.3 内存池](03-memory-pool.md) · 下一节：[第 3 章 迭代器与 traits](../ch03-iterators-traits/README.md)

## 为什么要学这个（先建立直觉）

在 C 里，分配内存和初始化是一步完成的——`calloc` 分配并清零，`malloc` + `memset` 分配并填充。但 C++ 的对象有构造函数，不能简单 memset。

```c
/* C: malloc + memset */
int* arr = malloc(n * sizeof(int));
memset(arr, 0, n * sizeof(int));  // 清零，对 int OK
// 但对 C++ 对象，memset 会破坏 vtable 指针！
```

```cpp
// C++: 分配内存 ≠ 构造对象
// 1. allocate → 分配原始内存（未构造）
// 2. construct → 在原始内存上调用构造函数（placement new）
// 两步分离是 RAII 和异常安全的基础

// STL 的 uninitialized_* 系列封装了"分配后构造"的过程
std::vector<int> v(100);
// vector 内部：
// 1. allocator.allocate(100) → 分配 400 字节原始内存
// 2. uninitialized_fill(start, start+100, 0) → 在原始内存上构造 100 个 int(0)
```

**直觉**：C++ 把"分配内存"和"构造对象"分成两步。`uninitialized_*` 是 STL 容器在原始内存上批量构造对象的标准手段。

## 这节讲什么

### 三个核心函数

```cpp
#include <memory>

// 1. uninitialized_copy: 把 [first, last) 拷贝到未构造内存 result
template<typename InputIter, typename ForwardIter>
ForwardIter uninitialized_copy(InputIter first, InputIter last, ForwardIter result);

// 2. uninitialized_fill: 在 [first, last) 上用 x 构造
template<typename ForwardIter, typename T>
void uninitialized_fill(ForwardIter first, ForwardIter last, const T& x);

// 3. uninitialized_fill_n: 在 [first, first+n) 上用 x 构造
template<typename ForwardIter, typename Size, typename T>
ForwardIter uninitialized_fill_n(ForwardIter first, Size n, const T& x);
```

### 内部实现（placement new）

```cpp
// 简化版 uninitialized_fill
template<typename ForwardIter, typename T>
void uninitialized_fill(ForwardIter first, ForwardIter last, const T& x) {
    for (; first != last; ++first) {
        ::new (static_cast<void*>(&*first)) T(x);  // placement new
    }
}
```

`::new (ptr) T(x)` = placement new = 在 `ptr` 指向的原始内存上调用 `T` 的拷贝构造函数。

### trivially constructible 特化

```cpp
// 对 trivially constructible 类型（如 int, char, POD），特化为 memset
template<>
void uninitialized_fill<int*, int>(int* first, int* last, const int& x) {
    // int 的拷贝构造是 trivial 的 → 等价于 memset
    std::fill(first, last, x);  // 编译器优化为 memset
}
```

**关键**：traits 萃取 `is_trivially_copyable<T>` 为 true 时，走 `memset`/`memmove`（零开销）；为 false 时走逐元素 placement new。

### 异常安全

```cpp
// uninitialized_fill 的异常安全版本（简化）
template<typename ForwardIter, typename T>
void uninitialized_fill(ForwardIter first, ForwardIter last, const T& x) {
    ForwardIter cur = first;
    try {
        for (; cur != last; ++cur) {
            ::new (static_cast<void*>(&*cur)) T(x);
        }
    } catch (...) {
        // 构造到一半抛异常 → 析构已构造的元素
        for (; first != cur; ++first) {
            first->~T();
        }
        throw;  // 重新抛出
    }
}
```

## 常见错误（新手踩坑）

### 错误 1：对非 POD 类型用 memset

```cpp
struct Widget {
    virtual void f() {}
    int data;
};
Widget* w = (Widget*)malloc(sizeof(Widget));
memset(w, 0, sizeof(Widget));  // 破坏 vtable 指针！UB！
// 应该用 placement new
new (w) Widget();  // 正确：调用构造函数
```

### 错误 2：忘记析构

```cpp
// placement new 构造的对象，必须手动析构
char buf[sizeof(Widget)];
Widget* w = new (buf) Widget();
// ... 使用 ...
w->~Widget();  // 必须手动析构！
// buf 本身不需要 free（栈上）
```

### 错误 3：在未构造内存上调用赋值

```cpp
int* p = (int*)malloc(100 * sizeof(int));
*p = 42;  // 对 int OK（trivially constructible）
// 但对非 trivially constructible 类型：
std::string* s = (std::string*)malloc(100 * sizeof(std::string));
s[0] = "hello";  // UB！s[0] 还没构造，operator= 依赖已构造状态
// 应该用 uninitialized_fill 或 placement new
new (s) std::string("hello");  // 正确
```

## 新手要点（和 C 的区别）

| 方面 | C | C++ STL |
|------|---|---------|
| 分配+初始化 | 一步（calloc/malloc+memset） | 两步（allocate + construct） |
| 构造方式 | memset（对所有类型） | placement new（对非 POD）/ memset（对 POD） |
| 异常安全 | 无（setjmp/longjmp 不析构） | try-catch 析构已构造元素 |
| 类型感知 | 无 | traits 萃取 trivially copyable |

## HFT 关联

- **POD 特化 = 零开销**：`uninitialized_copy` 对 int/float 等 trivially copyable 类型走 `memmove`，和手写 memcpy 等效
- **vector 扩容用 uninitialized_copy**：vector 扩容时在新内存上用 `uninitialized_copy` 构造元素，对 POD 走 memmove
- **自定义容器遵循两步模式**：HFT 自建容器分配内存后用 `uninitialized_*` 构造，保证异常安全和性能

## 代码自测

### Q1: placement new

```cpp
char buf[sizeof(int)];
int* p = new (buf) int(42);
std::cout << *p;
p->~int();  // 有必要吗？
```

<details>
<summary>答案</summary>

输出 **42**。

`p->~int()` **没必要**（但无害）。`int` 的析构函数是 trivial 的（什么也不做），调不调都行。

**但对非 trivial 析构类型必须调**：
```cpp
char buf[sizeof(std::string)];
std::string* s = new (buf) std::string("hello");
// ... 使用 ...
s->~basic_string();  // 必须调！否则内存泄漏（string 内部有堆分配）
```
</details>

### Q2: trivially copyable 特化

```cpp
struct Point { int x, y; };  // POD
struct Widget { Widget() {} virtual void f() {} int data; };  // 非 POD

Point* p1 = allocator<Point>().allocate(100);
uninitialized_fill(p1, p1 + 100, Point{0, 0});

Widget* p2 = allocator<Widget>().allocate(100);
uninitialized_fill(p2, p2 + 100, Widget{});
```
> p1 和 p2 的构造分别走什么路径？

<details>
<summary>答案</summary>

- **p1（Point = POD）**：`is_trivially_copyable<Point>` = true → 特化为 `memset`（或 `std::fill` → memset）。零开销。
- **p2（Widget = 非 POD，有虚函数）**：`is_trivially_copyable<Widget>` = false → 逐元素 placement new，调用 Widget 构造函数。每个元素都调 `Widget()` + 设置 vtable 指针。

**HFT**：热路径数据结构设计为 POD（无虚函数、无非 trivial 构造/析构），让 `uninitialized_*` 走 memmove 路径。
</details>

### Q3: 异常安全

```cpp
std::vector<Widget> v;
v.resize(1000000);  // 分配 + 构造 100 万个 Widget
```
> 如果第 500000 个 Widget 构造抛异常，vector 状态如何？

<details>
<summary>答案</summary>

**强异常保证**：vector 状态不变（像 resize 没被调过）。

内部流程：
1. 分配新内存（100 万个 Widget 的空间）
2. `uninitialized_fill` 逐个构造
3. 第 500000 个构造抛异常
4. catch 块：析构已构造的 499999 个 Widget
5. 释放新分配的内存
6. 异常向上传播

**结果**：vector 保持原状，无内存泄漏，无半构造状态。

**前提**：Widget 的析构函数不抛异常（C++ 标准要求析构不抛）。
</details>

### Q4: 自定义容器

```cpp
template<typename T>
class RingBuffer {
    T* data;
    size_t cap, head, tail;
public:
    RingBuffer(size_t n) : cap(n), head(0), tail(0) {
        data = (T*)operator new(n * sizeof(T));  // 原始内存
        // 不构造！延迟到 push 时构造
    }
    void push(const T& val) {
        new (data + tail) T(val);  // placement new
        tail = (tail + 1) % cap;
    }
    ~RingBuffer() {
        // 需要析构已 push 的元素
        // 但怎么知道哪些位置构造了？
    }
};
```
> 这个 RingBuffer 的析构函数怎么写？

<details>
<summary>答案</summary>

需要追踪哪些位置已构造（已 push 但未 pop 的元素）：

```cpp
~RingBuffer() {
    // 析构 [head, tail) 范围内的元素
    if (tail >= head) {
        for (size_t i = head; i < tail; i++)
            data[i].~T();
    } else {
        // 环形回绕
        for (size_t i = head; i < cap; i++)
            data[i].~T();
        for (size_t i = 0; i < tail; i++)
            data[i].~T();
    }
    operator delete(data);  // 释放原始内存
}
```

**关键**：`operator new` 分配的原始内存，必须手动 `placement new` 构造 + 手动析构 + `operator delete` 释放。不能用 `delete[]`（它会对所有位置调析构，但有些位置没构造）。

**HFT**：环形缓冲区是 HFT 常用数据结构（无锁队列底层），正确管理构造/析构是安全的基础。
</details>

## 参考与延伸

- 上一节：[2.3 内存池](03-memory-pool.md)
- 下一节：[第 3 章 迭代器与 traits](../ch03-iterators-traits/README.md)
