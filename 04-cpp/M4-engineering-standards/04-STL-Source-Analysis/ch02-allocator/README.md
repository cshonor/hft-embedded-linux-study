# 第 2 章 空间配置器

**Allocator**

## 本章讲什么

STL 的所有容器内存分配都走**配置器（allocator）**。SGI STL 的配置器分两级：一级直接走 `malloc`/`free`，二级用**内存池 + 自由链表（free-list）**管理小块内存，避免频繁 `malloc` 的碎片与锁开销。本章是理解 STL 容器内存模型的基础，也与 HFT 的 mempool 思想一脉相承。

## 要点

### 双级配置器

| 级别 | 触发条件 | 机制 |
|------|----------|------|
| 一级 | 请求 > 128 字节 | 直接 `malloc`/`free`，OOM 时调用户设的 `set_new_handler` |
| 二级 | 请求 ≤ 128 字节 | 16 个 free-list 桶（8/16/.../128 字节），内存池补给 |

**二级配置器的精妙**：把小块请求**向上取整**到 8 的倍数（如 12 字节 → 16 字节桶），用 free-list 维护已释放的同规格块。分配/回收是 O(1) 链表操作，无 `malloc` 锁、无碎片。

### free-list 结构

```cpp
union __obj { union __obj* free_list_link; char client_data[1]; };
```
用 `union` 复用内存——空闲时存 next 指针，分配后给用户存数据。零额外开销。

### 内存池（memory pool）

free-list 耗尽时，从内存池切一块（通常 20 个块的量）补给。内存池不足时调 `malloc` 补充。这种批量分配减少系统调用次数。

### `uninitialized_*` 系列

`uninitialized_fill`/`uninitialized_copy` 用 **placement new** 在未构造内存上构造对象，且对 trivially constructible 类型特化为 `memset`/`memmove`（零开销）。这是 STL 容器构造元素的标准手段。

## HFT 关联

- **free-list = HFT mempool 的原型**：SGI 二级配置器的 free-list 思想，正是 DPDK `rte_mempool`、HFT 订单对象池的基础——预分配 + 桶式 free-list，O(1) 分配回收、无锁（单线程）或原子操作（多线程）。
- **批量分配减少系统调用**：内存池批量 `malloc` 20 块，比每次 `malloc` 1 块少 19 次系统调用——HFT 启动预热就用了这个思路。
- **自定义 allocator 接 mempool**：`std::vector<T, MempoolAlloc<T>>` 让 STL 容器走 HFT mempool，热路径零 `malloc`。但 C++11 起要求 allocator 无状态才能跨容器安全共享——mempool allocator 要设计成无状态或用 `polymorphic_allocator`（PMR，C++17）。

## 自测题

1. SGI 配置器的一级和二级分别处理什么大小的请求？分界线是多少？
2. free-list 用 `union` 复用内存的原理是什么？为什么零额外开销？
3. 二级配置器如何减少 `malloc` 系统调用次数？
4. `uninitialized_fill` 对 trivially constructible 类型如何特化？为什么能零开销？
5. HFT 的 mempool 与 SGI free-list 的核心思想有何共性？

## 代码自测

### Q1: allocator 接口
```cpp
template<typename T>
class MempoolAlloc {
    MemoryPool<T>& pool;
public:
    using value_type = T;
    T* allocate(size_t n) { return pool.malloc(n); }
    void deallocate(T* p, size_t n) { pool.free(p, n); }
};

// 使用
MemoryPool<int> mp;
std::vector<int, MempoolAlloc<int>> v(MempoolAlloc<int>{mp});
v.push_back(42);
```
> 自定义分配器必须提供哪些接口？allocator_traits 的作用是什么？

<details>
<summary>答案与复习指引</summary>

**最小接口**（C++11 起）：
- `value_type` 类型别名
- `allocate(n)` → `T*`
- `deallocate(p, n)`

**`allocator_traits`** 提供默认实现：
- `construct(alloc, p, args...)` → 默认调 `::new (p) T(args...)`
- `destroy(alloc, p)` → 默认调 `p->~T()`
- `max_size()` → 默认 `size_t(-1) / sizeof(T)`

如果分配器没提供这些，traits 用默认实现。只需实现 `allocate`/`deallocate` 即可。

**HFT**：自定义 mempool 分配器避免 `operator new` 的锁/碎片，高频小对象（订单节点）用 `list<T, MempoolAlloc<T>>`。

**复习：** → [allocator 接口](./README.md)
</details>
