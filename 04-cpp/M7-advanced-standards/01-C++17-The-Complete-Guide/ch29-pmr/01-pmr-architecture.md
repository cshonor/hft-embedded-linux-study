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

<details>
<summary>参考答案</summary>

1. PMR 分三层：
   1. **`std::pmr::memory_resource`（接口层）**：抽象基类，定义内存分配的统一契约，只关心「字节数 + 对齐」。
   2. **具体资源（实现层）**：`new_delete_resource`、`null_memory_resource`、`monotonic_buffer_resource`、`synchronized_pool_resource`、`unsynchronized_pool_resource`，以及可自定义的派生类。
   3. **`std::pmr::polymorphic_allocator<T>`（适配层）**：把 `memory_resource*` 包装成标准 Allocator，供容器使用；它让「用了哪种资源」不进入容器类型。
一句话：资源负责内存，allocator 负责对接容器，容器类型因此保持统一。
2. 三个纯虚函数（用户派生时重写这三个，`memory_resource` 的公有 `allocate`/`deallocate`/`is_equal` 负责转发）：
```cpp
virtual void* do_allocate(std::size_t bytes, std::size_t alignment) = 0;
virtual void  do_deallocate(void* p, std::size_t bytes, std::size_t alignment) = 0;
virtual bool  do_is_equal(const memory_resource& other) const noexcept = 0;
```
`do_is_equal` 用来判断两个资源是否等价（等价的资源之间可以互相释放内存），是 allocator 相等性比较与容器 swap/移动语义的基础。
3. `std::allocator<T>` 是**静态、无状态**的：它的类型里不含任何资源信息，分配固定走 `::operator new`，也不需要（不能）持有状态。
`std::pmr::polymorphic_allocator<T>` 内部**持有一个 `memory_resource*`**，把分配通过虚函数的**运行期多态**转发给它；并且它符合 Allocator 要求（有 `allocate`/`deallocate`/`construct`/`destroy`、`select_on_container_copy_construction` 等）。
关键差别：前者「资源在类型里」（`std::allocator` 没有资源），后者「资源在运行期数据里」——所以所有 `polymorphic_allocator<T>` 都是同一个类型，容器类型也就统一了。
4. 因为 allocator 是容器**类型的一部分**：`std::vector<int, AllocA<int>>` 与 `std::vector<int, AllocB<int>>` 是两个不同的类型，语言层面没有为它们生成赋值运算符，所以不能互相赋值。
`std::pmr::vector<int>` 无论绑到哪个资源，类型都是 `std::vector<int, std::pmr::polymorphic_allocator<int>>`，分配器差异被**类型擦除**到运行期的 `memory_resource*`，因此两个 `pmr::vector<int>` 可以赋值。
赋值时的资源传播要注意：`polymorphic_allocator` 的 `propagate_on_container_copy_assignment` 为 `false`、`select_on_container_copy_construction()` 返回**默认构造**的分配器，所以**拷贝**不会沿用源容器的资源（目标继续用自己/默认资源），而**移动赋值**会传播资源；两个资源不等价的容器之间 `swap` 也是未定义行为。
5. 靠 **uses-allocator construction（分配器感知构造）** 传播：pmr 容器都是 allocator-aware 的，外层容器在构造元素时会把自己持有的 `polymorphic_allocator` **转换成元素类型对应的 `polymorphic_allocator<value_type>`** 并传给元素的构造函数。
```cpp
std::pmr::vector<std::pmr::string> vs(&mbr);
vs.emplace_back("hello");   // string 也从 mbr 分配
```
所以外层 vector、内层 string、乃至更深的嵌套都自动使用同一个 `memory_resource`，无需逐层手工指定。标准库内部用 `std::uses_allocator` 判断元素是否接受分配器参数来完成这件事。

</details>
