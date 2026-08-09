# 第 29 章 多态内存资源 PMR

**Polymorphic Memory Resources (PMR)**

## 本章讲什么

`std::pmr` 是 C++17 的内存分配框架——用**多态内存资源**（运行期分派）替代 `std::allocator`（编译期模板），让同一个容器在运行期切换分配器（mempool、monotonic、synchronized）。HFT 可控分配的核心工具。

## 要点

### 三层架构

```
1. memory_resource（抽象基类）
   ├── monotonic_buffer_resource：单调缓冲（只分配不释放，批量回收）
   ├── synchronized_pool_resource：线程安全的池
   ├── unsynchronized_pool_resource：非线程安全的池（快）
   └── 自定义资源（继承 memory_resource）

2. polymorphic_allocator<T>：使用 memory_resource 的分配器

3. pmr 容器：vector<T, polymorphic_allocator<T>> 等
```

### 基本用法

```cpp
#include <memory_resource>

// 1. 创建一个 monotonic buffer（栈上缓冲）
char buf[65536];
std::pmr::monotonic_buffer_resource mbr(buf, sizeof(buf));

// 2. 用它创建 pmr 容器
std::pmr::vector<int> v(&mbr);
v.reserve(1000);
for (int i = 0; i < 1000; ++i) v.push_back(i);   // 从 buf 分配，无 malloc

// 3. 容器嵌套：内层自动用同一资源
std::pmr::vector<std::pmr::string> vs(&mbr);
vs.emplace_back("hello");   // string 也从 buf 分配
```

### 四种内置资源

| 资源 | 特点 | 适用 |
|------|------|------|
| `monotonic_buffer_resource` | 只分配不释放，析构时一次回收 | 请求/事务作用域，最快 |
| `unsynchronized_pool_resource` | 池化，块大小分桶，单线程 | 单线程频繁分配释放 |
| `synchronized_pool_resource` | 同上但线程安全（有锁） | 多线程共享 |
| `null_memory_resource` | 永远抛异常 | 检测意外分配 |

### monotonic 的核心优势

```cpp
// 一个请求处理周期内的所有分配
void handle_request() {
    char buf[1 << 16];   // 栈上 64KB
    std::pmr::monotonic_buffer_resource mbr(buf, sizeof(buf));

    std::pmr::vector<Tick> ticks(&mbr);
    std::pmr::map<int, Order> orders(&mbr);
    std::pmr::string temp(&mbr);

    // 所有分配从 buf 取，无 malloc，无碎片
    // 函数返回时 mbr 析构，buf 自动"释放"（栈上无操作）
}
```

- **零 malloc**：所有分配从预分配缓冲取。
- **零碎片**：bump pointer 分配，无 free list。
- **批量回收**：析构即全部回收，无逐个 free。
- **不可中途释放**：单个对象不能单独 free，只能整体重置。

### 自定义 memory_resource

```cpp
class MemPoolResource : public std::pmr::memory_resource {
    void* do_allocate(size_t bytes, size_t align) override { /* 从 mempool 取 */ }
    void do_deallocate(void* p, size_t bytes, size_t align) override { /* 归还 */ }
    bool do_is_equal(const memory_resource& other) const noexcept override { return this == &other; }
};

MemPoolResource pool;
std::pmr::vector<int> v(&pool);   // 用自定义 mempool
```

## HFT 关联

- **monotonic 做请求作用域分配**：单笔订单处理的所有临时对象用 monotonic buffer（栈上 64KB），零 malloc、零碎片、批量回收。
- **pool_resource 做对象池**：订单对象频繁分配释放用 `unsynchronized_pool_resource`（单线程），块大小分桶减少碎片。
- **null_memory_resource 做护栏**：热路径入口设默认资源为 null，任何意外 malloc 立即抛异常，强制可控分配。
- **pmr 容器跨资源复用**：同一份策略代码，回测用 monotonic，生产用 mempool，只换资源不改编码。
- **避免 `synchronized_pool_resource`**：有锁，热路径不用。多线程各用独立 `unsynchronized_pool_resource`。
- **DPDK mempool 集成**：自定义 `memory_resource` 包装 DPDK `rte_mempool`，pmr 容器直接用网卡缓冲池。

## 自测题

1. PMR 的三层架构是什么？memory_resource、polymorphic_allocator、pmr 容器的关系？
2. `monotonic_buffer_resource` 的特点？为什么适合请求作用域？
3. `unsynchronized_pool_resource` 和 `synchronized_pool_resource` 的区别？HFT 用哪个？
4. `null_memory_resource` 在 HFT 中有什么用途？
5. 如何自定义 memory_resource 包装 DPDK mempool？

## 代码自测

### Q1: 多态内存资源
```cpp
// 传统：allocator 模板参数不同 = 不同类型
std::vector<int, MempoolAlloc<int>> v1;
std::vector<int, std::allocator<int>> v2;
// v1 和 v2 类型不同，不能互相赋值

// PMR：运行时多态分配器
pmr::synchronized_pool_resource pool;
pmr::vector<int> v3(&pool);  // 用 pool 分配
pmr::vector<int> v4;         // 用默认（new）
// v3 和 v4 类型相同（都是 pmr::vector<int>），可互相赋值
```
> PMR 解决了什么问题？pmr::vector 和 std::vector 有什么区别？

<details>
<summary>答案与复习指引</summary>

**PMR 解决的问题**：传统 allocator 是模板参数，不同 allocator = 不同类型，容器不能互相赋值/传参。PMR 用**运行时多态**分配器（`pmr::polymorphic_allocator`），所有 `pmr::vector<T>` 类型相同，只是内存来源不同。

**区别**：
- `std::vector<T>` — 模板参数固定 allocator，编译期决定
- `pmr::vector<T>` = `std::vector<T, pmr::polymorphic_allocator<T>>` — 运行时可切换内存资源

**PMR 内存资源**：
| 资源 | 特点 |
|------|------|
| `std::pmr::new_delete_resource` | 默认，走 operator new |
| `std::pmr::monotonic_buffer_resource` | 单调分配（只进不退，极快），适合临时容器 |
| `std::pmr::synchronized_pool_resource` | 线程安全的池分配 |
| `std::pmr::unsynchronized_pool_resource` | 非线程安全的池分配（更快） |

**HFT**：`monotonic_buffer_resource` 在栈/预分配 buffer 上分配，零 malloc、零碎片，适合每 tick 的临时容器。

**复习：** → [PMR](./README.md)
</details>
