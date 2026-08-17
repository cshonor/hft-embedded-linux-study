# C++17 对齐 new

## 修复方案

```cpp
struct alignas(64) CacheLine {
    int data[16];
};

// C++17：new 自动调用对齐版 operator new
CacheLine* p = new CacheLine;
// 内部调用：operator new(sizeof(CacheLine), std::align_val_t{64})
// 保证返回 64 字节对齐的指针

uintptr_t addr = reinterpret_cast<uintptr_t>(p);
assert((addr % 64) == 0);  // ✅ 保证对齐

delete p;
// 内部调用：operator delete(p, sizeof(CacheLine), std::align_val_t{64})
```

## align_val_t

```cpp
// C++17 新增：对齐值标签类型
namespace std {
    enum class align_val_t : size_t {};
}

// 新的 operator new 重载
void* operator new(std::size_t size, std::align_val_t align);
void operator delete(void* ptr, std::size_t size, std::align_val_t align);

// 显式使用
void* p = ::operator new(sizeof(CacheLine), std::align_val_t{64});
::operator delete(p, std::align_val_t{64});
```

## 自定义 operator new

```cpp
// C++17 要同时提供对齐版重载
void* operator new(std::size_t size) {
    return custom_alloc(size);
}
// 必须加这个重载：
void* operator new(std::size_t size, std::align_val_t align) {
    return custom_alloc_aligned(size, static_cast<size_t>(align));
}

void operator delete(void* p) noexcept {
    custom_free(p);
}
void operator delete(void* p, std::size_t size, std::align_val_t align) noexcept {
    custom_free_aligned(p, size, static_cast<size_t>(align));
}
```

## std::aligned_alloc

```cpp
// C++17 提供（C11 兼容）
void* std::aligned_alloc(std::size_t alignment, std::size_t size);
// 要求：alignment 是 2 的幂，size 是 alignment 的倍数（某些平台）

void* buf = std::aligned_alloc(64, 64 * 100);  // 100 个 64 字节块
std::free(buf);
```

## 验证对齐

```cpp
struct alignas(64) HotData { int x, y, z; };

auto* p = new HotData;
assert(reinterpret_cast<uintptr_t>(p) % 64 == 0);  // C++17 保证

// STL 容器也对齐
std::vector<HotData> v;
v.push_back({});
assert(reinterpret_cast<uintptr_t>(v.data()) % 64 == 0);  // C++17 保证
```

## 自测题

1. C++17 如何修复超对齐 `new` 的问题？
2. `std::align_val_t` 是什么？为什么用标签类型而不是直接传 size_t？
3. 自定义 `operator new` 在 C++17 要加什么重载？
4. `std::aligned_alloc` 的参数要求是什么？
5. C++17 的 STL 容器对超对齐元素保证对齐吗？
