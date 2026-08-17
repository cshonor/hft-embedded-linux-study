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
