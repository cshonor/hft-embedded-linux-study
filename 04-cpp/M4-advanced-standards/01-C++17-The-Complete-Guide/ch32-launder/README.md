# 第 32 章 std::launder

**std::launder()**

## 本章讲什么

`std::launder` 是 C++17 的底层工具——解决 placement new 后编译器优化导致的"指针假设失效"问题。日常代码很少直接用，但实现内存池、variant、optional 这类底层工具时必需。

## 要点

### 问题：placement new 与编译器假设

```cpp
struct X { const int n; };

X x{1};
const int* p = &x.n;        // p 指向 x.n == 1
new (&x) X{2};               // placement new：原地把 x 重建为 2
// 现在 *p 是什么？
```

编译器可能认为 `*p` 仍是 1——因为 `x.n` 是 `const`，编译器假设它不变，把 `*p` 缓存到寄存器。但 placement new 改了它。C++17 之前这是未定义行为，C++17 用 `launder` 解决。

### `std::launder` 的作用

```cpp
X x{1};
new (&x) X{2};
const int* p = std::launder(&x.n);   // launder 告诉编译器：重新读，别用缓存假设
assert(*p == 2);
```

`launder` 是个编译器屏障——告诉编译器"这个指针指向的对象可能已被替换，不要做基于旧值的优化"。运行时通常零开销（只是阻止优化）。

### 什么时候需要 launder

| 场景 | 需要 launder？ |
|------|----------------|
| placement new 重建 const 成员 | 需要 |
| placement new 重建引用成员 | 需要 |
| placement new 重建普通非 const 成员 | 通常不需要（编译器不假设不变） |
| 普通 new/delete | 不需要 |
| 修改非 const 变量 | 不需要 |
| 实现内存池/optional/variant | 需要（内部 placement new） |

### 标准库内部如何用

`std::optional`、`std::variant` 内部用 `aligned_storage` + placement new 存值，访问时需要 `launder` 确保 UB-free。C++17 之前这些库的实现在技术上依赖 UB（虽然实际工作），C++17 launder 让它们合法化。

### 典型用法：mempool

```cpp
class MemPool {
    alignas(T) unsigned char buf[sizeof(T)];
public:
    template <typename... Args>
    T* construct(Args&&... args) {
        new (buf) T(std::forward<Args>(args)...);
        return std::launder(reinterpret_cast<T*>(buf));
    }
    void destroy(T* p) {
        p->~T();
    }
};
```

placement new 后用 `launder` 返回指针，确保编译器不做错误假设。

## HFT 关联

- **mempool 实现必需**：HFT 自写 mempool 用 placement new + `launder`，确保返回的指针合法。
- **对象复用**：mempool 中槽位复用（析构 + placement new 重建），`launder` 保证新对象指针正确。
- **日常业务代码不用**：写策略、行情处理不直接用 launder——它是库作者的工具。
- **理解 optional/variant 的正确性**：知道标准库内部靠 launder 保证 placement new 的合法性。
- **零运行开销**：launder 运行时无操作，只是阻止编译器优化，HFT 可放心用。

## 自测题

1. `std::launder` 解决什么问题？不用会怎样？
2. placement new 重建 const 成员为什么需要 launder？
3. 普通 new/delete 需要 launder 吗？为什么？
4. 哪些场景需要 launder？（mempool、optional、variant...）
5. launder 有运行时开销吗？HFT 为什么可放心用？

## 代码自测

### Q1: placement new 与 launder
```cpp
struct Widget {
    int x;
    Widget(int v) : x(v) {}
};

alignas(Widget) unsigned char buf[sizeof(Widget)];
new (buf) Widget(42);  // placement new

// C++17 前：未定义行为（编译器可能缓存旧值）
// Widget* p = reinterpret_cast<Widget*>(buf);
// std::cout << p->x;  // 可能输出垃圾值

// C++17: launder 告诉编译器"这个指针指向新对象"
Widget* p = std::launder(reinterpret_cast<Widget*>(buf));
std::cout << p->x;  // 42，保证正确
```
> 为什么 placement new 后不能直接用 reinterpret_cast？launder 解决了什么？

<details>
<summary>答案与复习指引</summary>

**问题**：C++ 对象模型规定，一个存储位置上只能有一个活跃对象。`placement new` 在 `buf` 上构造了新 Widget，但编译器可能不知道 `reinterpret_cast<Widget*>(buf)` 指向的是新对象——它可能假设 `buf` 还是 unsigned char 数组，做错误的优化（缓存旧值）。

**`std::launder`**：告诉编译器"这个指针可能指向一个与之前不同的对象，不要做基于旧类型的假设"。相当于"清洗"指针，消除编译器的优化假设。

**何时需要 launder**：
- placement new 后访问新对象
- 通过 unsigned char buffer 构造对象

**何时不需要**：
- 普通 `new` 返回的指针（编译器知道是新对象）
- `std::vector` 的内存（vector 内部已处理）

**HFT**：内存池/placement new 是常用技巧，launder 确保正确性。但大多数场景用 `std::aligned_storage` 或 `std::optional` 更安全。

**复习：** → [launder](./README.md)
</details>
