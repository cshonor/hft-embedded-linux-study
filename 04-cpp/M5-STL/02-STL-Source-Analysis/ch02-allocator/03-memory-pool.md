# 2.3 内存池机制

> 第 2 章 空间配置器 · 第 3 节 · 上一节：[2.2 free-list 结构](02-free-list-structure.md) · 下一节：[2.4 uninitialized 系列](04-uninitialized-series.md)

## 为什么要学这个（先建立直觉）

在 C 里，每次 `malloc(16)` 都是一次系统调用（或至少一次库函数调用）。如果你需要 20 个 16 字节的块，就是 20 次 malloc。SGI 内存池的做法是：一次 malloc 320 字节（20×16），切成 20 块挂到 free-list。

```c
/* C: 20 次 malloc */
for (int i = 0; i < 20; i++) {
    ptrs[i] = malloc(16);  // 20 次系统调用
}
```

```cpp
// SGI: 1 次 malloc 切 20 块
// free-list 空时 → refill(16) → 从内存池切 20 个 16 字节块
// 内存池不够 → malloc(20 * 16 + extra) → 切块
// 1 次系统调用，20 块分配
```

**直觉**：批量分配减少系统调用次数。内存池是大块预分配 + 按需切割的缓冲区。

## 这节讲什么

### 内存池数据结构

```cpp
// SGI 内存池（简化）
static char* start_free = nullptr;  // 内存池起点
static char* end_free = nullptr;    // 内存池终点

// 内存池可用空间 = end_free - start_free
```

### refill 流程（free-list 空时补充）

```cpp
void* refill(size_t n) {
    // 尝试从内存池切 20 个 n 字节块
    int nobjs = 20;
    char* chunk = chunk_alloc(n, nobjs);  // 切割，nobjs 可能被修改

    if (nobjs == 1) return chunk;  // 只切出 1 块，直接返回

    // 切出多块：第 1 块返回给用户，其余挂到 free-list
    __obj** my_free_list = free_list + freelist_index(n);
    __obj* result = (__obj*)chunk;
    __obj* current_obj = (__obj*)(chunk + n);
    *my_free_list = current_obj;

    for (int i = 1; i < nobjs - 1; i++) {
        __obj* next_obj = (__obj*)((char*)current_obj + n);
        current_obj->free_list_link = next_obj;
        current_obj = next_obj;
    }
    current_obj->free_list_link = nullptr;
    return result;
}
```

### chunk_alloc 流程（从内存池切割）

```cpp
char* chunk_alloc(size_t size, int& nobjs) {
    size_t total_bytes = size * nobjs;
    size_t bytes_left = end_free - start_free;  // 内存池剩余

    if (bytes_left >= total_bytes) {
        // 内存池够 20 块 → 全部切出
        char* result = start_free;
        start_free += total_bytes;
        return result;
    } else if (bytes_left >= size) {
        // 不够 20 块但够一些 → 切能切的
        nobjs = bytes_left / size;
        total_bytes = size * nobjs;
        char* result = start_free;
        start_free += total_bytes;
        return result;
    } else {
        // 内存池连 1 块都不够 → 补充内存池
        // 1. 把剩余碎片挂到合适的 free-list
        // 2. malloc 新内存（2 * 20 * size + 额外）
        // 3. 如果 malloc 失败 → 从更大的 free-list 桶借
        size_t bytes_to_get = 2 * total_bytes + round_up(heap_size >> 4);
        start_free = (char*)malloc(bytes_to_get);
        // ... 处理失败和碎片 ...
        return chunk_alloc(size, nobjs);  // 递归重试
    }
}
```

### 内存池增长策略

```
第 1 次 refill(16):
  内存池空 → malloc(2 * 20 * 16 + extra) = malloc(640+)
  切 20 个 16 字节块 → 1 个给用户，19 个挂 free-list[1]
  剩余内存池 = 640 - 320 = 320 字节

第 2 次 refill(16)（19 个用完后）:
  内存池有 320 字节 → 切 20 个 16 字节 = 320 字节 → 刚好 20 块
  内存池空 → 下次再 malloc
```

## 常见错误（新手踩坑）

### 错误 1：以为内存池会自动收缩

```cpp
// 回收 1000 个小对象
// free-list 里堆积了 1000 个块
// 但内存不会还给 OS
// 进程内存占用不会降低
```

### 错误 2：混淆 SGI 内存池和现代 PMR

```cpp
// SGI: 编译期固定，全局单例
// PMR: 运行时可配置，每个 resource 独立
std::pmr::monotonic_buffer_resource mbr;  // 单调增长，不回收
std::pmr::unsynchronized_pool_resource pool;  // 类似 SGI free-list
```

### 错误 3：多线程下用非线程安全的配置器

SGI 原版配置器非线程安全。现代 libstdc++ 的 SGI 风格 pool 有线程安全版本（加锁），但性能不如无锁方案。

## 新手要点（和 C 的区别）

| 方面 | C (malloc) | SGI (内存池) |
|------|-----------|-------------|
| 系统调用 | 每次分配 1 次 | 20 块才 1 次 |
| 碎片 | 严重 | 小块无碎片 |
| 内存归还 | free 立即归还 | 不归还（直到进程结束） |
| 多线程 | malloc 有锁 | free-list 可无锁（单线程） |

## HFT 关联

- **预热消除运行时 malloc**：HFT 启动时预分配所有对象池，运行时零 malloc
- **批量分配减少系统调用**：和 DPDK `rte_mempool` 的 `mempool_create` 预分配 N 个对象思路一致
- **内存只增不减**：HFT 系统内存预算固定，不归还 OS 是可接受的

## 代码自测

### Q1: 批量分配

```cpp
// free_list[1]（16 字节桶）为空
// refill(16) 被调用
// nobjs 初始为 20
// 内存池有 200 字节剩余
```
> chunk_alloc 会切出几个块？返回什么？

<details>
<summary>答案</summary>

`bytes_left = 200`, `total_bytes = 16 * 20 = 320`。

200 < 320，但 200 >= 16 → 切 `200 / 16 = 12` 块。

```cpp
nobjs = 200 / 16;  // = 12
total_bytes = 16 * 12;  // = 192
// 切出 12 块，内存池剩余 200 - 192 = 8 字节（碎片）
```

refill 拿到 12 块：
- 第 1 块返回给用户
- 第 2-12 块（11 块）挂到 free_list[1]

下次 free_list[1] 空时，内存池只有 8 字节（不够 1 个 16 字节块），会触发 malloc 补充。
</details>

### Q2: 碎片处理

```cpp
// 内存池剩余 8 字节，但请求 16 字节
// chunk_alloc(16, 20) 被调用
// bytes_left = 8 < 16
```
> 这 8 字节碎片怎么处理？

<details>
<summary>答案</summary>

8 字节碎片不会被浪费——它被挂到 free_list[0]（8 字节桶）：

```cpp
// 把 8 字节碎片挂到 free_list[0]
if (bytes_left > 0) {
    __obj** my_free_list = free_list + freelist_index(bytes_left);
    ((__obj*)start_free)->free_list_link = *my_free_list;
    *my_free_list = (__obj*)start_free;
}
```

然后 malloc 新内存补充内存池，递归调用 `chunk_alloc` 重试。

**效果**：碎片被"零存整取"——8 字节碎片虽然不能给 16 字节请求用，但能服务未来的 8 字节请求。
</details>

### Q3: 内存不归还

```cpp
// 进程运行中释放了大量小对象
// free-list 堆积了 1GB 空闲块
// 但 RSS（进程内存）不降低
```
> 为什么？HFT 中这有问题吗？

<details>
<summary>答案</summary>

**为什么**：SGI free-list 回收只把块挂回链表，不调 `free()`。内存池的 `start_free`/`end_free` 只增不减（除非显式 `purge`）。

**HFT 中是否有问题**：
- **通常不是问题**：HFT 系统启动时预热分配固定大小的池，运行时分配/回收量稳定，不会无限增长
- **可能的问题**：如果设计不当（如不同大小的小对象交替分配释放），某些桶堆积不用但其他桶频繁 malloc
- **解决方案**：HFT 通常用固定大小的对象池（如订单对象池全是一种大小），避免碎片

**对比**：现代 `std::pmr::monotonic_buffer_resource` 也是只增不减，适合 arena 分配模式。
</details>

### Q4: 自定义内存池

```cpp
// HFT 订单对象池（简化）
class OrderPool {
    std::vector<Order> storage;  // 预分配
    std::vector<size_t> free_list;  // 空闲索引
public:
    OrderPool(size_t n) : storage(n), free_list(n) {
        for (size_t i = 0; i < n; i++) free_list[i] = i;
    }
    Order* alloc() {
        size_t idx = free_list.back();
        free_list.pop_back();
        return &storage[idx];
    }
    void free(Order* p) {
        size_t idx = p - &storage[0];
        free_list.push_back(idx);
    }
};
```
> 这个池和 SGI free-list 有什么共同点？

<details>
<summary>答案</summary>

**共同点**：
1. **预分配**：启动时分配 N 个对象，运行时不再 malloc
2. **O(1) 分配/回收**：free-list 栈式操作（SGI 用链表，这里用 vector 栈）
3. **不归还 OS**：内存池固定，回收只是标记空闲
4. **零碎片**：所有块大小相同（Order），无取整浪费

**区别**：
- SGI 按大小分桶（16 种），这里只有 1 种（Order）
- SGI 用 union 复用内存做链表，这里用单独的 vector 存索引
- SGI 内存池可动态增长，这里固定大小

**HFT**：订单对象池通常用这种固定大小方案——启动预热 N 个订单槽位，运行时 O(1) 分配回收，零 malloc，cache 友好（连续存储）。
</details>

## 参考与延伸

- 上一节：[2.2 free-list 结构](02-free-list-structure.md)
- 下一节：[2.4 uninitialized 系列](04-uninitialized-series.md)
