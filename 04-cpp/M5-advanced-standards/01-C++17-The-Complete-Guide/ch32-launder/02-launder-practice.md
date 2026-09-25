# launder 实践

## 内存池中的 launder

```cpp
template <typename T, size_t N>
class MemPool {
    alignas(T) unsigned char buf[N][sizeof(T)];
    bool used[N] = {};

public:
    template <typename... Args>
    T* construct(size_t idx, Args&&... args) {
        new (buf[idx]) T(std::forward<Args>(args)...);
        used[idx] = true;
        // launder 确保返回的指针合法
        return std::launder(reinterpret_cast<T*>(buf[idx]));
    }

    void destroy(size_t idx) {
        T* p = std::launder(reinterpret_cast<T*>(buf[idx]));
        p->~T();  // 析构
        used[idx] = false;
    }

    T* get(size_t idx) {
        if (!used[idx]) return nullptr;
        return std::launder(reinterpret_cast<T*>(buf[idx]));
    }
};
```

## optional 的简化实现

```cpp
template <typename T>
class MyOptional {
    alignas(T) unsigned char buf[sizeof(T)];
    bool has_value = false;

public:
    template <typename... Args>
    void emplace(Args&&... args) {
        if (has_value) destroy();
        new (buf) T(std::forward<Args>(args)...);
        has_value = true;
    }

    T& value() {
        // 必须 launder：buf 上可能有新对象
        return *std::launder(reinterpret_cast<T*>(buf));
    }

    void destroy() {
        value().~T();
        has_value = false;
    }
};
```

## C++17 前的 workaround

```cpp
// C++14：union 替代（合法但冗长）
template <typename T>
union Storage {
    T val;
    char dummy;
    Storage() : dummy() {}
    ~Storage() {}
};

// C++14：编译器扩展（如 GCC 的 __builtin_launder）
auto* p = __builtin_launder(reinterpret_cast<T*>(buf));

// C++17：标准化
auto* p = std::launder(reinterpret_cast<T*>(buf));
```

## 何时用 launder 的检查清单

```
需要 launder：
☑ placement new 在 unsigned char buffer 上构造
☑ placement new 后访问 const 成员
☑ placement new 后访问引用成员
☑ 实现内存池/对象池
☑ 实现 optional/variant 类似工具

不需要 launder：
☐ 普通 new 返回的指针
☐ placement new 后访问非 const 普通成员（通常安全）
☐ STL 容器内部（已处理）
☐ 直接在正确类型的存储上 placement new
```

## 自测题

1. 内存池中 placement new 后返回指针，为什么需要 launder？
2. 简化的 `MyOptional` 中 `value()` 为什么要 launder？
3. C++17 前如何替代 launder？
4. 访问非 const 普通成员需要 launder 吗？为什么通常不需要？
5. 列出需要和不需要 launder 的场景。

<details>
<summary>参考答案</summary>

1. 内存池提供的是**裸存储**（`alignas(T) std::byte[]` 或 `void*`），对象由 placement new 创建。池返回给用户的指针通常只能通过对存储做 `reinterpret_cast` 得到——这个转换本身不"指向"新对象，编译器仍认为那里是裸字节，通过该指针访问 `T` 是未定义行为。
`std::launder` 把这个指针变成"指向该地址上当前存在的 `T` 对象"的合法指针，从而让后续访问有定义。若池直接返回并保存 placement new 的返回值，则不必 launder；但只要是从存储重新计算/转换出的指针，就需要。
2. 因为 `MyOptional` 的存储（如 `alignas(T) std::byte buf[sizeof(T)]`）可以被**反复 emplace**：每次都在同一地址上创建**一个新的 `T` 对象**。
`value()` 每次都用 `reinterpret_cast<T*>(buf)` 取指针，编译器完全有理由认为「这个指针指向的还是上一次那个对象」，从而缓存先前读过的值、或复用基于旧对象不变性的优化。`std::launder` 断开这种假设，保证每次 `emplace` 后读到的是**当前**对象。
3. C++17 之前没有标准手段，常见替代：
   - 用 **union** 存储（`union { T val; char dummy; }`），靠 union 的成员规则规避——合法但写起来冗长、对非平凡类型要手写构造/析构；
   - 用**编译器扩展**：GCC/Clang 的 `__builtin_launder`、MSVC 的相应内置；
   - 或者依赖宽松的别名规则编译（如 `-fno-strict-aliasing`）——不可移植，且会牺牲优化。
C++17 把这件事标准化为 `std::launder`，从此有了可移植写法。
4. 通常不需要。原因在标准 [basic.life] 的"透明可替换"规则：如果原对象**不是 const 限定**、且（若是类类型）**不含 const 限定或引用类型的非_static 数据成员**，那么在原形存储上创建的新对象可以被旧的指针/引用/名字直接指代，编译器不会基于"值不可变"做假设。
```cpp
struct Z { int n; };
Z z{1};
new (&z) Z{2};   // z.n 不是 const，读 z.n 得到 2
```
所以访问普通非 const 成员是安全的。但只要涉及 const/引用成员、或指针是从 `unsigned char` 缓冲 `reinterpret_cast` 得来的，就必须 launder——判据始终是"我手上的指针是否真的指向当前这个对象"。
5. **需要 launder**：
   - placement new 在 `unsigned char` / `std::byte` 缓冲上构造对象后，从缓冲转换出指针；
   - placement new 重建对象后要访问 **const 成员**或**引用成员**；
   - 自己实现内存池 / 对象池（从裸存储返回对象指针）；
   - 自己实现 `optional` / `variant` 这类"原地重建"的工具。
**不需要 launder**：
   - 普通 `new` 返回的指针；
   - placement new **本身返回**的指针（直接用它即可）；
   - 在正确类型的存储上 placement new 后访问**非 const 普通成员**（透明可替换）；
   - STL 容器内部（实现已处理，用户不用管）。

</details>
