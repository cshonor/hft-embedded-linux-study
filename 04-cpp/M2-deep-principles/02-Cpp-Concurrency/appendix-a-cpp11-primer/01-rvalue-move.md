# A.1 右值引用与移动语义

> 附录 A C++11 精要 · 上一章：[11.6 常见陷阱](../ch11-testing-debugging/06-pitfalls.md) · 下一节：[A.2 lambda 表达式](02-lambda.md)

## 这节讲什么

C++11 引入右值引用（`T&&`）和移动语义——让"偷资源"代替"拷贝资源"成为可能。本节讲左值/右值的区别、移动构造函数、`std::move` 的本质、以及移动语义在并发中的价值（`std::thread` 转移所有权）。

---

## 核心规则（代码+表格）

### 左值 vs 右值

```cpp
int x = 10;
// x 是左值（有名字、可取地址）
// 10 是右值（临时的、即将销毁）

std::vector<int> make_vec() { return std::vector<int>(1000, 1); }
std::vector<int> v = make_vec();
// v 是左值，make_vec() 的返回值是右值（临时对象）

// 左值引用 T&：绑定到左值
int& ref = x;  // OK

// 右值引用 T&&：绑定到右值
int&& rref = 10;  // OK
// int& bad = 10;  // 错误：左值引用不能绑定右值

// 常量左值引用可以绑定右值（特殊规则）
const int& cref = 10;  // OK
```

### 移动构造函数

```cpp
class Buffer {
    char* data;
    size_t size;
public:
    // 拷贝构造：深拷贝 O(n)
    Buffer(const Buffer& other) : size(other.size) {
        data = new char[size];
        memcpy(data, other.data, size);
    }

    // 移动构造：偷资源 O(1)
    Buffer(Buffer&& other) noexcept
        : data(other.data), size(other.size) {
        other.data = nullptr;  // 源对象置空
        other.size = 0;
    }

    ~Buffer() { delete[] data; }
};

Buffer make_buffer(size_t n) {
    return Buffer(n);  // 返回右值 → 移动构造（或 NRVO）
}

Buffer a(1024);
Buffer b = std::move(a);  // 移动构造：a 的资源偷给 b，a 变空
```

### `std::move` 的本质

```cpp
// std::move 不移动任何东西！它只是把左值转成右值引用
template <typename T>
typename std::remove_reference<T>::type&& move(T&& t) noexcept {
    return static_cast<typename std::remove_reference<T>::type&&>(t);
}
// 真正的移动由移动构造函数完成

int x = 10;
int&& r = std::move(x);  // r 是右值引用，但 x 的值没变
// move 只是 cast，让移动构造函数/赋值被选中
```

### 移动语义在并发中的价值

```cpp
// std::thread 的所有权转移
std::thread t1([]{ work(); });
std::thread t2 = std::move(t1);  // t1 的线程所有权转移给 t2
// t1 现在是 not-a-thread，t2 管理线程
// 这让 thread 可以放进容器
std::vector<std::thread> threads;
threads.emplace_back([]{ work1(); });
threads.emplace_back([]{ work2(); });

// promise 的移动
std::promise<int> p;
std::future<int> f = p.get_future();
// promise 可以 move 到另一个线程
std::thread t([&p = p]{
    p.set_value(42);  // p 被 move 捕获
});
```

---

## 新手要点（和 C 的区别）

- **C 没有移动语义**：C 里"传指针"就是一种原始的"移动"——但不安全（指针可能被多处使用）。C++ 的移动语义是"类型安全的资源转移"——移动后源对象置空，不会 double-free。
- **`std::move` 是 cast 不是操作**：C 程序员可能以为 `std::move(x)` 会"移动 x 的内容"——不是。它只是把 `x` 转成右值引用，让移动构造函数被调用。真正的移动由移动构造函数完成。
- **移动构造函数要 `noexcept`**：C 程序员可能不知道 `noexcept` 的重要性——`std::vector` 在扩容时，如果元素的移动构造不是 `noexcept`，会退化为拷贝（因为拷贝失败可以回退，移动失败不行）。移动构造一定要标 `noexcept`。
- **`T&&` 不总是右值引用**：在模板中 `T&&` 是"万能引用"（forwarding reference），可能绑定左值或右值。C 程序员转型 C++ 时要理解 `std::forward` 的完美转发——这是 C++ 的高级特性。

---

## HFT 关联

- **移动语义减少 HFT 中的深拷贝**：行情包、订单对象如果在容器间传递，移动语义让传递 O(1) 而非 O(n)——这对延迟敏感的 HFT 至关重要。
- **`std::thread` 放入 `std::vector`**：HFT 系统的线程池需要把 `std::thread` 存入容器——这靠移动语义实现（thread 不可拷贝但可移动）。
- **`noexcept` 移动构造是 HFT 的硬要求**：HFT 中的热路径对象（如行情缓冲）如果放进 `std::vector`，移动构造必须 `noexcept`——否则扩容时退化为拷贝，延迟抖动。
- **SPSC 队列存指针 vs 移动值**：HFT 的 SPSC 队列可以存值（靠移动语义高效传递）或存指针（零拷贝）。选择取决于对象大小——小对象移动，大对象指针。

---

## 自测题

1. 左值和右值的区别是什么？`int& ` 和 `int&&` 分别绑定什么？
2. `std::move` 做了什么？它本身移动了任何东西吗？
3. 移动构造函数为什么要标 `noexcept`？不标会怎样？
4. `std::thread` 为什么可以放进 `std::vector`？靠什么实现？
5. 在 HFT 中，移动语义如何减少延迟？

---

## 参考与延伸

- 下一节：[A.2 lambda 表达式](02-lambda.md)
- 上一章：[11.6 常见陷阱](../ch11-testing-debugging/06-pitfalls.md)
- 回到：[附录 A](README.md)
