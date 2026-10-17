# monotonic_buffer_resource 详解

## 核心特性

```cpp
#include <memory_resource>

// 栈上缓冲
char buf[65536];
std::pmr::monotonic_buffer_resource mbr(buf, sizeof(buf));

// 或堆上缓冲
std::pmr::monotonic_buffer_resource mbr2(1 << 20);  // 1MB，内部 new

// 或上游资源链
std::pmr::monotonic_buffer_resource mbr3(4096, std::pmr::new_delete_resource());
```

**特点**：
- **只分配不释放**：`deallocate` 是空操作，不回收内存
- **bump pointer**：分配就是指针前移 `ptr += size`
- **零碎片**：没有 free list，没有空洞
- **批量回收**：析构时一次性释放所有内存
- **极快**：分配 O(1)，一次比较 + 加法

## 分配过程

```
初始状态：
buf: [....................free....................]
     ^ptr

allocate(8):
buf: [xxxxxxxx................free................]
            ^ptr

allocate(16):
buf: [xxxxxxxxxxxxxxxxxxxxxxxx....free............]
                                ^ptr

deallocate(p, 8):  ← 空操作！指针不回退
buf: [xxxxxxxxxxxxxxxxxxxxxxxx....free............]
                                ^ptr

析构 → 整个 buf 标记为可用
```

## 多缓冲链式

```cpp
// 第一个缓冲用完后，自动从上游资源申请新缓冲
std::pmr::monotonic_buffer_resource mbr(
    4096,  // 初始缓冲大小
    std::pmr::new_delete_resource()  // 上游：用 new 分配新缓冲
);

std::pmr::vector<int> v(&mbr);
for (int i = 0; i < 10000; ++i) v.push_back(i);
// 第一个 4KB 用完后，自动从 new_delete 申请下一个 4KB
// 析构时所有缓冲一次性释放
```

## 请求作用域模式

```cpp
void handle_request(const Request& req) {
    // 栈上 64KB，零 malloc
    alignas(64) char buf[64 * 1024];
    std::pmr::monotonic_buffer_resource mbr(buf, sizeof(buf));

    // 所有临时对象从 buf 分配
    std::pmr::vector<Tick> ticks(&mbr);
    std::pmr::map<int, Order> orders(&mbr);
    std::pmr::string temp(&mbr);

    // 处理逻辑...
    // 所有分配 O(1)，零 malloc、零碎片

    // 函数返回 → mbr 析构 → buf 自动回收（栈上无操作）
}
// 下一笔请求复用同一栈空间
```

## 不可中途释放

```cpp
std::pmr::monotonic_buffer_resource mbr(4096);
std::pmr::vector<int> v(&mbr);
v.resize(100);  // 分配了 400 字节

v.clear();      // 容器清空，但 deallocate 是空操作
v.resize(100);  // 又分配 400 字节——不会复用之前的空间！
// monotonic 不回收单个对象，内存只会增长

// 如果需要频繁分配释放，用 pool_resource 而非 monotonic
```

## 自测题

1. `monotonic_buffer_resource` 的分配算法是什么？为什么 O(1)？
2. `deallocate` 在 monotonic 中是什么操作？为什么？
3. 初始缓冲用完后怎么办？上游资源是什么？
4. 请求作用域模式的优势是什么？
5. 为什么说 monotonic "不可中途释放"？需要频繁释放该用什么？

<details>
<summary>参考答案</summary>

1. 它是**bump / arena 分配器**：资源持有当前缓冲区和「已用位置」指针，分配时把指针按对齐要求向上调整后返回旧位置，并把指针推到 `p + bytes`。
因为只有「指针加法 + 对齐取整」两步，没有空闲链表查找、没有大小分桶、没有合并逻辑，所以是 **O(1)**，且分配出的地址在缓冲内连续递增——cache 局部性也最好。
2. 标准规定 `monotonic_buffer_resource::do_deallocate` 的效果为 **none**，即空操作：它不回收单个块，内存只在资源被析构时一次性释放。
原因正是「单调」：资源只增不减，一旦允许回收就要维护空闲链表或做块合并，也就失去了 O(1) 与零碎片的性质。个别实现可能对「刚分配的最后一块」做特殊处理，但**不可依赖**。
3. 初始缓冲耗尽后，它会向**构造时指定的上游资源**（`upstream_resource`）申请一块**更大的新缓冲**继续 bump（典型实现按倍数增长，具体增长策略标准未规定），旧缓冲仍被保留直到资源析构。
上游默认是 `std::pmr::new_delete_resource()`（即用 `::operator new` 分配）；也可以显式指定，例如把 `unsynchronized_pool_resource` 作为上游，或指定 `null_memory_resource()` 让超出缓冲时直接抛 `std::bad_alloc`。
4. 请求作用域（request-scope arena）模式：在处理函数里于**栈上**放一块缓冲区，构造 `monotonic_buffer_resource`，让本次请求的所有临时容器都从它分配，函数返回时资源析构、栈空间自动回收。
优势：**零 malloc**（前 N 字节完全不走系统分配器）、分配 O(1)、**无内存碎片**、容器地址集中在栈上/连续区域（cache 局部性好）、生命周期与请求一致因而不会泄漏，下一笔请求直接复用同一栈空间。
5. 因为它的 `deallocate` 是空操作，且内存只随分配单调增长：即使容器 `clear()` 甚至析构，字节也不会回到资源里，下次分配照样往后推。
```cpp
v.resize(100);  // 分配 400 字节
v.clear();      // deallocate 是空操作
v.resize(100);  // 又分配 400 字节，不复用旧空间
```
所以长生命周期 + 频繁分配释放的场景要用 **`synchronized_pool_resource` / `unsynchronized_pool_resource`**（分桶 + 空闲链表，支持单块回收）；monotonic 适合「一批分配、整体释放」的场景（请求作用域、批处理）。

</details>
