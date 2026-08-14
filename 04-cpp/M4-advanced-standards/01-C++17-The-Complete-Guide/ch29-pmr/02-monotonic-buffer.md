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
