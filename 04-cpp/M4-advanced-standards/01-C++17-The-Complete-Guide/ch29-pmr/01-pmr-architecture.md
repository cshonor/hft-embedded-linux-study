# PMR 三层架构

## 架构总览

```
┌─────────────────────────────────────────┐
│  pmr 容器（pmr::vector, pmr::string...） │  ← 第三层：用户接口
├─────────────────────────────────────────┤
│  polymorphic_allocator<T>               │  ← 第二层：分配器适配
├─────────────────────────────────────────┤
│  memory_resource（抽象基类）             │  ← 第一层：内存来源
│  ├── monotonic_buffer_resource          │
│  ├── unsynchronized_pool_resource       │
│  ├── synchronized_pool_resource         │
│  └── null_memory_resource               │
└─────────────────────────────────────────┘
```

## 第一层：memory_resource

```cpp
#include <memory_resource>

// 抽象基类，三个虚函数
class memory_resource {
public:
    void* allocate(size_t bytes, size_t alignment = alignof(max_align_t));
    void deallocate(void* p, size_t bytes, size_t alignment = alignof(max_align_t));
    bool is_equal(const memory_resource& other) const noexcept;
protected:
    virtual void* do_allocate(size_t, size_t) = 0;
    virtual void do_deallocate(void*, size_t, size_t) = 0;
    virtual bool do_is_equal(const memory_resource&) const noexcept = 0;
};
```

**设计**：运行时多态——容器通过 `memory_resource*` 指针调用分配，实际分配策略在运行期决定。

## 第二层：polymorphic_allocator

```cpp
// polymorphic_allocator 是 std::allocator 的多态版本
template <typename T>
class polymorphic_allocator {
    memory_resource* resource_;  // 指向内存资源
public:
    T* allocate(size_t n) {
        return static_cast<T*>(resource_->allocate(n * sizeof(T), alignof(T)));
    }
    void deallocate(T* p, size_t n) {
        resource_->deallocate(p, n * sizeof(T), alignof(T));
    }
    // ...
};

// pmr::vector 就是 vector<T, polymorphic_allocator<T>>
namespace pmr {
    template <typename T>
    using vector = std::vector<T, std::pmr::polymorphic_allocator<T>>;
}
```

**关键**：所有 `pmr::vector<T>` 类型相同（不同于不同 allocator 模板的 vector），可以互相赋值。

## 第三层：pmr 容器

```cpp
// 创建资源
std::pmr::monotonic_buffer_resource mbr(buf, sizeof(buf));

// 用资源创建容器
std::pmr::vector<int> v(&mbr);       // vector 从 mbr 分配
std::pmr::string s(&mbr);            // string 从 mbr 分配
std::pmr::map<int, std::pmr::string> m(&mbr);  // map 和内层 string 都从 mbr 分配

// 嵌套容器：内层自动用同一资源
std::pmr::vector<std::pmr::string> vs(&mbr);
vs.emplace_back("hello");  // string 也从 mbr 分配（通过传播）
```

## 与传统 allocator 的对比

```cpp
// 传统：不同 allocator = 不同类型
std::vector<int, AllocA<int>> v1;
std::vector<int, AllocB<int>> v2;
// v1 和 v2 类型不同，不能互相赋值

// PMR：同一类型，不同运行时资源
std::pmr::vector<int> v3(&pool_a);
std::pmr::vector<int> v4(&pool_b);
v3 = v4;  // ✅ 同类型，可以赋值（但分配器不同，传播语义）
```

## 自测题

1. PMR 的三层架构分别是什么？
2. `memory_resource` 的三个虚函数是什么？
3. `polymorphic_allocator` 和 `std::allocator` 的区别？
4. 为什么 `pmr::vector<T>` 和 `std::vector<T, AllocA<T>>` 不能互相赋值，但两个 `pmr::vector<T>` 可以？
5. 嵌套 pmr 容器如何自动传播内存资源？
