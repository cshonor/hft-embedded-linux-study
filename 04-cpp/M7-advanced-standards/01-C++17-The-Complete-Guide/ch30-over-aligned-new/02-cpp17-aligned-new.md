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

<details>
<summary>参考答案</summary>

1. C++17 引入了**带对齐参数的 `operator new` / `operator delete` 重载**，并让 `new` 表达式在类型的对齐要求超过 `__STDCPP_DEFAULT_NEW_ALIGNMENT__`（默认新对齐，通常等于 `alignof(max_align_t)`）时**自动选择对齐版本**：
```cpp
struct alignas(64) CacheLine { int data[16]; };
CacheLine* p = new CacheLine;   // 调用 operator new(sizeof, align_val_t{64})
delete p;                       // 调用 operator delete(p, sizeof, align_val_t{64})
```
于是 `reinterpret_cast<std::uintptr_t>(p) % 64 == 0` 成为**标准保证**，不再依赖平台 API 或手写偏移。配套还新增了 `std::align_val_t`、`std::aligned_alloc`，以及 `std::allocator` 等库组件的对齐支持。
2. 它是 C++17 新增的一个**强枚举标签类型**：
```cpp
namespace std { enum class align_val_t : std::size_t {}; }
```
用它而不是裸 `std::size_t`，是为了**给 `operator new` 的重载集一个明确、不会误配的参数类型**：`operator new` 已经是重载重灾区（`nothrow` 版、placement 版、数组版、用户自定义版），如果再塞一个 `operator new(size_t, size_t)`，很容易与「大小相近」的其他重载或用户自定义的 placement new 混淆，也可能把对齐值误解释成别的含义。
标签类型让 `operator new(sizeof(T), std::align_val_t{64})` 的意图一目了然，且不与任何既有重载冲突。
3. 必须补齐**对齐版本**的重载，否则超对齐类型的 `new` 会找不到匹配而退回到不带对齐的重载（导致不对齐）或编译失败：
```cpp
void* operator new(std::size_t size, std::align_val_t align);
void* operator new[](std::size_t size, std::align_val_t align);

void operator delete(void* p, std::size_t size, std::align_val_t align) noexcept;
void operator delete[](void* p, std::size_t size, std::align_val_t align) noexcept;
```
实践中通常与不带对齐的四个重载（`operator new(size)`、`operator new[](size)`、`operator delete(p)`、`operator delete[](p)`）成对提供，并且 `delete` 的签名要与 `new` **一一对应**，否则释放走错路径。
4. `void* std::aligned_alloc(std::size_t alignment, std::size_t size)`（C++17 提供，源自 C11）：
   - `alignment` 必须是**有效的对齐值**（实现支持、且通常是 2 的幂）；否则行为未定义。
   - `size` 是请求的总字节数；C11 要求 `size` 是 `alignment` 的**整数倍**（各标准版本/实现对此的要求有过调整），实践中按整数倍传入最稳妥。
   - 返回的指针必须用 **`std::free`** 释放，不能用 `delete`（配套关系不同）。
```cpp
void* buf = std::aligned_alloc(64, 64 * 100);   // 100 个 64 字节块
std::free(buf);
```
5. 保证。C++17 起 `std::allocator<T>::allocate` 会走对齐版 `::operator new`（当 `alignof(T)` 超过默认新对齐时使用 `align_val_t`），所以标准容器对超对齐元素也提供对齐保证：
```cpp
std::vector<HotData> v;      // HotData 是 alignas(64)
v.push_back({});
assert(reinterpret_cast<std::uintptr_t>(v.data()) % 64 == 0);  // C++17 保证
```
自定义 allocator 则需要自己调用对齐版分配函数才能保持这个保证；另外 `std::pmr::memory_resource` 的 `do_allocate(bytes, alignment)` 本身就带对齐参数。

</details>
