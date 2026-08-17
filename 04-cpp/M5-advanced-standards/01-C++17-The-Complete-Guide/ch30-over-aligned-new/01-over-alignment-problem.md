# 超对齐问题背景

## 什么是超对齐

```cpp
// 普通对齐：max_align_t（通常 8 或 16 字节）
struct Normal {
    int x, y;  // alignof(Normal) == 4
};

// 超对齐：超过 max_align_t
struct alignas(64) CacheLine {  // 64 字节对齐
    int data[16];
};

struct alignas(128) PageAligned {  // 128 字节对齐
    char data[128];
};
```

**超对齐**（over-alignment）：类型的对齐要求超过 `max_align_t`（通常 8 或 16 字节）。常见场景：
- Cache 行对齐（64 字节）：避免 false sharing
- SIMD 对齐（32/64 字节）：AVX/AVX-512
- DMA 对齐（128/256 字节）：网卡缓冲
- 页对齐（4096 字节）：内存映射

## C++14 的问题

```cpp
struct alignas(64) HotData {
    int x, y, z;
};

// C++14：new 不保证 64 对齐！
HotData* p = new HotData;
// operator new(size) 只保证 alignof(max_align_t) 对齐
// 可能返回 16 字节对齐的指针 → HotData 不在 cache 行边界！

// 验证：
uintptr_t addr = reinterpret_cast<uintptr_t>(p);
bool is_64_aligned = (addr % 64) == 0;
// C++14：可能 false！
```

**后果**：
- `alignas(64)` 白写——实际不对齐
- Cache 行跨界 → false sharing
- SIMD 加载崩溃（`_mm256_load_ps` 要求 32 对齐）
- DMA 失败或性能下降

## C++14 的 workaround

```cpp
// C++14 手动对齐分配
void* raw = std::malloc(sizeof(HotData) + 64);  // 多分配 64 字节
void* aligned = reinterpret_cast<void*>(
    (reinterpret_cast<uintptr_t>(raw) + 63) & ~63ULL  // 对齐到 64
);
HotData* p = new (aligned) HotData;  // placement new

// 析构 + 释放
p->~HotData();
std::free(raw);

// 或用平台 API
posix_memalign(&ptr, 64, sizeof(HotData));  // POSIX
_aligned_malloc(sizeof(HotData), 64);        // MSVC
```

**问题**：冗长、易错、不跨平台。

## 自测题

1. 什么是超对齐？`max_align_t` 通常是多少？
2. C++14 的 `new` 对 `alignas(64)` 类型有什么问题？
3. 超对齐在 HFT 中有哪些应用场景？
4. C++14 如何手动对齐分配？有什么缺点？
5. 超对齐不对齐会导致什么后果？
