# 2.2 free-list 结构与 union 复用

> 第 2 章 空间配置器 · 第 2 节 · 上一节：[2.1 双级配置器](01-dual-level-allocator.md) · 下一节：[2.3 内存池](03-memory-pool.md)

## 为什么要学这个（先建立直觉）

在 C 里，管理空闲内存块通常需要额外的链表节点——每个块要多花 8 字节存 next 指针。SGI STL 用 union 技巧让内存块自己当链表节点，零额外开销。

```c
/* C: 传统 free-list 需要额外节点 */
struct free_node {
    struct free_node* next;  // 8 字节开销
    /* ... 用户数据 ... */
};
// 每个空闲块要多占 8 字节存 next 指针
```

```cpp
// SGI: union 复用——空闲时存 next，分配后存数据
union obj {
    union obj* free_list_link;  // 空闲时：指向下一个空闲块
    char client_data[1];        // 分配后：用户数据覆盖此处
};
// 同一块内存，两种用途，零额外开销！
```

**直觉**：内存块空闲时不需要存数据，那它的空间可以用来存 next 指针。分配给用户后，next 指针的位置变成用户数据。一块内存两用。

## 这节讲什么

### union 复用原理

```cpp
// SGI free-list 节点
union __obj {
    union __obj* free_list_link;  // 当块在 free-list 中时：存下一个块的地址
    char client_data[1];          // 当块被分配出去后：用户数据从这里开始
};

// free-list 是一个指针数组
__obj* free_list[16];  // 16 个桶，每个桶是一条单链表
```

**生命周期**：

```
空闲状态：
  free_list[i] → [obj] → [obj] → [obj] → NULL
                 ↓       ↓       ↓
              link      link    link     （存 next 指针）

分配后：
  free_list[i] → [obj] → [obj] → NULL
                 ↓
              user data  （用户数据覆盖 link 位置）
```

### allocate 流程

```cpp
void* allocate(size_t n) {
    if (n > 128) return malloc(n);  // 大块走一级
    __obj** my_free_list = free_list + freelist_index(n);
    __obj* result = *my_free_list;
    if (result == nullptr) {
        return refill(round_up(n));  // free-list 空，从内存池补充
    }
    *my_free_list = result->free_list_link;  // 取头节点，更新链表头
    return result;  // 返回这块内存（用户看到的是 client_data）
}
```

### deallocate 流程

```cpp
void deallocate(void* p, size_t n) {
    if (n > 128) { free(p); return; }  // 大块走 free
    __obj** my_free_list = free_list + freelist_index(n);
    __obj* q = (__obj*)p;
    q->free_list_link = *my_free_list;  // 把释放的块插到链表头
    *my_free_list = q;
    // O(1) 回收，无系统调用
}
```

### 为什么 union 安全

```cpp
// 一块内存的两种状态（互斥）：
// 状态 1（空闲）：union 存 free_list_link（next 指针）
// 状态 2（已分配）：union 存 client_data（用户数据）
// 这两种状态不会同时存在 → union 复用安全
```

关键条件：**空闲块不存用户数据，已分配块不需要 next 指针**。两者生命周期互斥。

## 常见错误（新手踩坑）

### 错误 1：担心 union 会损坏数据

```cpp
// 疑问：用户数据不会覆盖 next 指针吗？
// 回答：不会。分配出去的块已经从 free-list 摘除了，不需要 next。
// 下次回收时，用户数据已经析构（对 POD 无所谓），next 覆盖回数据区。
```

### 错误 2：以为现代 STL 还用 union 复用

现代 libstdc++ 的 `std::allocator` 直接走 `operator new`/`delete`，不用 free-list。PMR 内存池内部用类似但不完全相同的机制。

### 错误 3：忘记对齐

```cpp
// union 的对齐 = 最大成员的对齐
// free_list_link 是指针 → 8 字节对齐（64 位）
// 所以分配的内存至少 8 字节对齐 → 满足大多数类型需求
```

## 新手要点（和 C 的区别）

| 方面 | C (传统 free-list) | SGI (union 复用) |
|------|-------------------|-----------------|
| 额外开销 | 每块 +8 字节 next | 零（复用数据区） |
| 复杂度 | O(1) | O(1) |
| 对齐 | 取决于节点结构 | 天然 8 字节对齐 |
| 安全条件 | 无 | 空闲/已分配互斥 |

## HFT 关联

- **union 复用 = 零开销链表**：HFT 对象池（订单/消息）用相同技巧，空闲时存 next，使用时存数据
- **侵入式链表**：Linux `list_head` 也是类似思想——把链表节点嵌入对象内部而非额外分配
- **对齐保证**：union 天然保证 8 字节对齐，满足 HFT 对 cache line 对齐的基本需求

## 代码自测

### Q1: union 复用原理

```cpp
union __obj {
    union __obj* free_list_link;
    char client_data[1];
};
// 假设 free_list[0] 指向一个 8 字节块
__obj* p = free_list[0];
free_list[0] = p->free_list_link;  // 摘除头节点
// 现在把 p 返回给用户
// 用户写入 8 字节数据 → 会覆盖什么？
```

<details>
<summary>答案</summary>

用户写入的 8 字节数据覆盖 `free_list_link` 的位置。

这是安全的，因为：
1. 这个块已经从 free-list 摘除（`free_list[0]` 已指向下一个块）
2. 用户使用期间，这个块不在 free-list 中，不需要 next 指针
3. 用户释放时，`deallocate` 重新设置 `free_list_link`，覆盖回用户数据（此时用户数据已不再需要）

**关键**：空闲和已分配是互斥状态，同一块内存两种用途不冲突。
</details>

### Q2: deallocate 流程

```cpp
void deallocate(void* p, size_t n) {
    // n = 16, 对应 free_list[1]
    __obj* q = (__obj*)p;
    q->free_list_link = free_list[1];  // A
    free_list[1] = q;                   // B
}
```
> A 和 B 两步分别在做什么？

<details>
<summary>答案</summary>

- **A 步**：把释放的块 `q` 的 `free_list_link` 指向当前链表头 `free_list[1]` → 把 q 插到链表头部
- **B 步**：更新链表头 `free_list[1]` 指向 `q` → q 成为新的头节点

效果：`free_list[1]` → `q` → `旧头` → ... → NULL

这是**头插法**，O(1) 回收，无遍历。
</details>

### Q3: 为什么 union 对齐重要

```cpp
struct alignas(64) CacheLineAligned {  // 需要 64 字节对齐
    int data[16];
};
// SGI free-list 的 union 对齐是多少？够吗？
```

<details>
<summary>答案</summary>

SGI free-list 的 union 对齐 = `sizeof(union __obj*)` = 8 字节（64 位指针）。

**不够**！`CacheLineAligned` 需要 64 字节对齐，但 free-list 只提供 8 字节对齐。

**HFT**：如果需要 cache line 对齐（64 字节），不能用 SGI 二级配置器。需要：
1. 自定义 allocator 用 `aligned_alloc`/`posix_memalign` 分配
2. 或用 C++17 `std::aligned_alloc`
3. 或 `operator new` + `alignas`

```cpp
struct AlignedAlloc {
    using value_type = CacheLineAligned;
    CacheLineAligned* allocate(size_t n) {
        return (CacheLineAligned*)::operator new(n * sizeof(CacheLineAligned),
            std::align_val_t(64));
    }
};
```
</details>

### Q4: 空闲链表 vs 内存池

```cpp
// SGI: free-list 回收时不真正释放内存
deallocate(p, 16);  // p 回到 free_list[1]，不调 free()
// 内存池只会增长不会缩小（除非显式 purge）
```
> 这种策略有什么优缺点？

<details>
<summary>答案</summary>

**优点**：
- 回收 O(1)，无系统调用
- 同大小的下次分配直接命中 free-list，O(1)
- 适合高频小对象分配/回收的场景

**缺点**：
- 内存只增不减（进程不释放给 OS）
- 不同大小的块不互通（8 字节桶的空闲块不能给 16 字节请求用）
- 长期运行可能内存占用偏高

**HFT**：HFT 启动时预热分配够用的小对象池，运行时零 malloc/free。进程结束时统一释放。内存只增不减是可接受的——HFT 系统内存预算通常固定。
</details>

## 参考与延伸

- 上一节：[2.1 双级配置器](01-dual-level-allocator.md)
- 下一节：[2.3 内存池](03-memory-pool.md)
