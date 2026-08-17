# 2.1 SGI 双级配置器

> 第 2 章 空间配置器 · 第 1 节 · 上一节：[本章概览](README.md) · 下一节：[2.2 free-list 结构](02-free-list-structure.md)

## 为什么要学这个（先建立直觉）

在 C 里，内存分配就一条路：`malloc`/`free`。小对象频繁 malloc 会导致内存碎片和锁竞争。

```c
/* C: 所有分配走 malloc */
char* p1 = malloc(8);   // 8 字节
char* p2 = malloc(16);  // 16 字节
char* p3 = malloc(12);  // 12 字节（实际分配可能 16+，对齐+元数据开销）
free(p1); free(p2); free(p3);
// 问题：频繁 malloc → 碎片 + 锁竞争
```

```cpp
// SGI STL: 双级配置器，小对象走内存池
// 大对象（>128字节）→ 一级配置器 → malloc/free
// 小对象（≤128字节）→ 二级配置器 → free-list + 内存池
```

**直觉**：SGI 把小对象分配从"每次 malloc"变成"从预分配的 free-list 取"，O(1) 分配回收、无 malloc 锁、无碎片。

## 这节讲什么

### 双级配置器架构

```
alloc(size)
  ├─ size > 128 → 一级配置器
  │                 └─ malloc(size)
  │                 └─ 失败 → set_new_handler → 重试
  └─ size ≤ 128 → 二级配置器
                    └─ free_list[round_up(size)/8 - 1]
                    └─ 有空闲块 → 返回，O(1)
                    └─ 无空闲块 → 从内存池切 20 块 → 返回
```

### 一级配置器（> 128 字节）

```cpp
// 简化版一级配置器
class malloc_alloc {
public:
    static void* allocate(size_t n) {
        void* result = malloc(n);
        if (!result) {
            // OOM 处理：调 set_new_handler，重试
            result = oom_malloc(n);
        }
        return result;
    }
    static void deallocate(void* p, size_t) {
        free(p);
    }
};
```

一级配置器就是 `malloc`/`free` 的包装，加了 OOM 处理（`set_new_handler` 回调）。

### 二级配置器（≤ 128 字节）

```cpp
// 简化版二级配置器
class default_alloc {
    enum { ALIGN = 8, MAX_BYTES = 128, NFREELISTS = 16 };
    static void* free_list[NFREELISTS];  // 16 个桶

    static size_t round_up(size_t bytes) {
        return (bytes + ALIGN - 1) & ~(ALIGN - 1);  // 向上取整到 8 的倍数
    }
    static size_t freelist_index(size_t bytes) {
        return (bytes + ALIGN - 1) / ALIGN - 1;  // 8→0, 16→1, ..., 128→15
    }
public:
    static void* allocate(size_t n) {
        if (n > MAX_BYTES) return malloc_alloc::allocate(n);  // 大块走一级
        size_t idx = freelist_index(n);
        void* result = free_list[idx];
        if (result) {
            free_list[idx] = *(void**)result;  // 取下链表头
            return result;
        }
        // free-list 空 → 从内存池补充
        return refill(round_up(n));
    }
};
```

### 16 个 free-list 桶

| 桶索引 | 块大小 | 处理请求范围 |
|--------|--------|-------------|
| 0 | 8 字节 | 1-8 字节 |
| 1 | 16 字节 | 9-16 字节 |
| 2 | 24 字节 | 17-24 字节 |
| ... | ... | ... |
| 15 | 128 字节 | 121-128 字节 |

请求 12 字节 → `round_up(12) = 16` → 桶 1 → 返回 16 字节块（4 字节浪费但无碎片）。

## 常见错误（新手踩坑）

### 错误 1：不知道 STL 容器内部走配置器

```cpp
std::list<int> l;
l.push_back(42);
// list 节点 = sizeof(node) ≈ 24 字节
// SGI: 走二级配置器 → free-list[2]（24 字节桶）
// 现代 libstdc++: 走 operator new → malloc
```

### 错误 2：以为现代 STL 还用 SGI 双级配置器

SGI 双级配置器是历史设计。现代 libstdc++ 默认用 `std::allocator` → `operator new` → `malloc`。SGI 的 free-list 思想被 C++17 PMR (`polymorphic_allocator`) 和 `std::pmr::synchronized_pool_resource` 继承。

### 错误 3：自定义 allocator 忘了 deallocate

```cpp
template<typename T>
class BadAlloc {
public:
    using value_type = T;
    T* allocate(size_t n) { return (T*)malloc(n * sizeof(T)); }
    // 忘了 deallocate → 内存泄漏！
};
```

## 新手要点（和 C 的区别）

| 方面 | C (malloc) | SGI STL (双级) |
|------|-----------|---------------|
| 小对象 | 直接 malloc | free-list O(1) |
| 大对象 | 直接 malloc | 一级 = malloc |
| 碎片 | 严重 | 小块无碎片（取整复用） |
| 锁竞争 | 有 | 无（单线程）/原子（多线程） |
| OOM 处理 | 返回 NULL | set_new_handler 重试 |

## HFT 关联

- **free-list = HFT mempool 原型**：SGI 二级配置器的 free-list 思想是 DPDK rte_mempool、HFT 订单池的基础
- **批量分配减少系统调用**：内存池一次 malloc 20 块，比 20 次 malloc 少 19 次系统调用
- **PMR 替代 SGI 配置器**：C++17 `std::pmr::pool_options` 提供标准化的内存池配置

## 代码自测

### Q1: 配置器选择

```cpp
// 以下分配分别走哪级配置器？（SGI STL 语境）
std::list<int> l;       // list 节点 ≈ 24 字节
std::vector<char> v;    // vector 扩容到 256 字节
```

<details>
<summary>答案</summary>

- **list 节点（24 字节）**：≤ 128 → 二级配置器 → free-list[2]（24 字节桶）
- **vector 扩容（256 字节）**：> 128 → 一级配置器 → malloc

list 每个节点走 free-list，O(1) 分配回收。vector 的连续数组走 malloc。

**HFT**：list 的节点分配虽然 O(1)，但非连续内存导致 cache miss。vector 的 malloc 是一次性大块，连续但可能有延迟尖峰（reserve 消除）。
</details>

### Q2: round_up

```cpp
// SGI 的 round_up 函数
static size_t round_up(size_t bytes) {
    return (bytes + 7) & ~7;  // 向上取整到 8 的倍数
}
```
> round_up(1)、round_up(12)、round_up(128) 分别是多少？

<details>
<summary>答案</summary>

- `round_up(1)` = 8
- `round_up(12)` = 16
- `round_up(128)` = 128

`& ~7` 把低 3 位清零（向下取整到 8 倍数），`+7` 保证向上取整。

**意义**：所有 ≤128 字节的请求被归类到 16 个固定大小的桶（8/16/24/.../128），同一桶的块可以互相复用，无碎片。
</details>

### Q3: 自定义 allocator 最小接口

```cpp
template<typename T>
class MyAlloc {
    // 必须提供哪些成员？
};
```

<details>
<summary>答案</summary>

C++11 最小接口：

```cpp
template<typename T>
class MyAlloc {
public:
    using value_type = T;  // 必须
    T* allocate(size_t n) { /* 返回 n*sizeof(T) 字节内存 */ }
    void deallocate(T* p, size_t n) { /* 释放 */ }
};
```

`allocator_traits` 提供其他操作的默认实现：
- `construct(alloc, p, args...)` → `::new (p) T(args...)`
- `destroy(alloc, p)` → `p->~T()`

只需实现 allocate/deallocate，其余自动默认。
</details>

### Q4: PMR 内存池

```cpp
// C++17 PMR 内存池
std::pmr::synchronized_pool_resource pool;
std::pmr::vector<int> v(&pool);  // vector 走内存池
v.push_back(42);
```
> PMR 内存池和 SGI 二级配置器有什么共性？

<details>
<summary>答案</summary>

**共性**：
1. **按大小分桶**：PMR 也有 size class（类似 SGI 的 8/16/.../128 桶）
2. **free-list 回收**：释放的块回到 free-list，下次同大小请求直接复用
3. **批量分配**：free-list 空时从上游 allocator（通常 malloc）批量切多块

**区别**：
- SGI 是编译期固定 16 桶，PMR 可运行时配置（`pool_options`）
- SGI 是 SGI 专用，PMR 是 C++17 标准
- PMR 支持有状态（`polymorphic_allocator` 携带 resource 指针），SGI 要求无状态

**HFT**：PMR 是现代 C++ 内存池的标准方案，替代 SGI 双级配置器。
</details>

## 参考与延伸

- 上一节：[本章概览](README.md)
- 下一节：[2.2 free-list 结构](02-free-list-structure.md)
