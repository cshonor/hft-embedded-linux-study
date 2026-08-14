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
