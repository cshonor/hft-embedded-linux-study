# <memory> 改进

## std::align

```cpp
#include <memory>

// 在缓冲区内做对齐
char buf[1024];
size_t space = sizeof(buf);
void* ptr = buf;

// 在 buf 中找 64 字节对齐的位置
void* aligned = std::align(64, sizeof(Obj), ptr, space);
if (aligned) {
    // ptr 指向对齐地址，space 是剩余空间
    auto obj = new (ptr) Obj();  // placement new
}
```

**参数**：
- `alignment`：对齐字节数（如 64 for cache line）
- `size`：需要的对象大小
- `ptr`：输入缓冲区指针，输出对齐后指针
- `space`：输入缓冲区大小，输出剩余空间

## std::launder（详见第 32 章）

```cpp
// placement new 后的指针屏障
alignas(int) unsigned char buf[sizeof(int)];
new (buf) int(42);

// C++17 前：直接 cast 可能被优化器搞乱
// int* p = reinterpret_cast<int*>(buf);  // UB（编译器可能假设 buf 不是 int）

// C++17：launder 告诉编译器"这个指针真的指向新对象"
int* p = std::launder(reinterpret_cast<int*>(buf));
// *p == 42，安全
```

## uninitialized 系列算法

```cpp
#include <memory>

// uninitialized_default_construct：在未初始化内存上默认构造
std::allocator<Widget> alloc;
Widget* p = alloc.allocate(10);
std::uninitialized_default_construct(p, p + 10);  // 调用 10 次 Widget()

// uninitialized_value_construct：值初始化
std::uninitialized_value_construct(p, p + 10);  // 调用 Widget() 值初始化

// uninitialized_default_construct_n
std::uninitialized_default_construct_n(p, 10);

// destroy / destroy_n
std::destroy(p, p + 10);       // 调用 10 次 ~Widget()
std::destroy_n(p, 10);

// uninitialized_move / uninitialized_move_n
std::vector<Widget> src = /* ... */;
std::uninitialized_move(src.begin(), src.end(), p);  // 移动构造到 p
```

## 实际应用

```cpp
// 自定义容器的构造/析构
template <typename T>
class SimpleVector {
    T* data_;
    size_t size_;
public:
    SimpleVector(size_t n) : data_(std::allocator<T>{}.allocate(n)), size_(n) {
        std::uninitialized_default_construct_n(data_, n);
    }
    ~SimpleVector() {
        std::destroy_n(data_, size_);
        std::allocator<T>{}.deallocate(data_, size_);
    }
};

// HFT：预分配对齐内存池
class AlignedPool {
    alignas(64) char buf[64 * 1024];  // 64KB，64 字节对齐
    void* ptr = buf;
    size_t space = sizeof(buf);
public:
    template <typename T>
    T* alloc() {
        if (std::align(64, sizeof(T), ptr, space)) {
            auto p = static_cast<T*>(ptr);
            ptr = static_cast<char*>(ptr) + sizeof(T);
            space -= sizeof(T);
            return new (p) T();
        }
        return nullptr;
    }
};
```

## 自测题

1. `std::align` 的作用是什么？参数含义？
2. `std::launder` 解决什么问题？什么时候需要？
3. `uninitialized_default_construct` 和 `uninitialized_value_construct` 的区别？
4. `std::destroy_n` 做什么？
5. HFT 如何用 `std::align` 做 cache 行对齐内存池？

<details>
<summary>参考答案</summary>

1. `std::align(alignment, size, ptr, space)` 在给定的缓冲区里**找到第一个满足对齐要求的地址**并返回。
参数含义：`alignment` —— 要求的对齐值（必须是 2 的幂）；`size` —— 要放置的对象字节数；`ptr` —— 输入/输出的 `void*&`，传入缓冲区当前起始地址，返回对齐后的地址；`space` —— 输入/输出的 `size_t&`，传入缓冲区剩余字节数，返回对齐后剩余的字节数。
返回值：成功返回对齐后的指针（等于调整后的 `ptr`），空间不够则返回 `nullptr`（并且此时 `ptr`/`space` 不被修改）。
2. 它解决「**在已有存储里重新创建对象后，如何合法地拿到指向新对象的指针**」的问题。
典型场景：用 placement new 在旧对象的存储上构造新对象（尤其旧类型是 const 限定的、或含 const 成员/引用成员），或者从 `std::optional`/小缓冲里取出新构造的对象——直接沿用旧指针是未定义行为，编译器可以假定那个指针仍指向旧对象。
```cpp
alignas(T) std::byte buf[sizeof(T)];
T* p = new (buf) T{};
p->~T();
T* q = new (buf) T{};      // 复用存储
q = std::launder(q);       // 告诉编译器：这里现在是新对象
```
只有在「存储被复用、旧指针仍要继续使用」这类场合才需要；普通的 placement new 直接用 new 返回的指针即可，不必 `launder`。
3. 区别在于被构造元素的初始化方式：
   - `uninitialized_default_construct` 做**默认初始化**（等价于 `::new (p) T;`）：对类类型调用默认构造函数，对 `int`/`double` 等平凡类型**不初始化**，值是未定的。
   - `uninitialized_value_construct` 做**值初始化**（等价于 `::new (p) T();`）：对类类型调用默认构造函数，对平凡类型**清零**。
所以想让 `int` 数组拿到确定的 0，要用 `uninitialized_value_construct`（或对应的 `_n` 版本），用 default 版本读到的值是不确定的。
4. `std::destroy_n(first, n)` 对 `[first, first + n)` 范围内的 n 个已构造对象**按序调用析构函数**（`std::destroy(first, last)` 的计数版本）。
对平凡类型（trivially destructible）它什么都不做（可被优化掉）；对类类型则逐个析构，但不释放存储——释放仍由 allocator/deallocate 负责。它常与 `uninitialized_*` 系列配对，用于手写容器的析构。
5. 思路是：先用 `alignas` 让整块缓冲区按 cache line 对齐，再用 `std::align` 在块内逐个切出对齐的对象槽位：
```cpp
class AlignedPool {
    alignas(64) std::byte buf[64 * 1024];   // 64KB，按 cache line 对齐
    void*  ptr   = buf;
    std::size_t space = sizeof(buf);
public:
    template <typename T>
    T* alloc() {
        if (std::align(64, sizeof(T), ptr, space)) {   // 对齐到 64 字节
            auto p = static_cast<T*>(ptr);
            ptr   = static_cast<std::byte*>(ptr) + sizeof(T);
            space -= sizeof(T);
            return new (p) T();          // placement new，注意自行 destroy
        }
        return nullptr;                  // 空间不足
    }
};
```
这样每个对象都独占一条 cache line（或至少不跨界），避免伪共享（false sharing）；配合 `uninitialized_*` / `destroy_n` 管理生命周期即可。

</details>
