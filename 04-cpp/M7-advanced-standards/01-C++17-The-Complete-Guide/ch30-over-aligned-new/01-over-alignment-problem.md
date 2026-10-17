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

<details>
<summary>参考答案</summary>

1. **超对齐**（over-aligned）指类型的对齐要求**大于** `alignof(std::max_align_t)`（即大于实现保证的「最大平凡对齐」）。
`std::max_align_t` 的对齐值由实现决定，主流 x86-64 平台（GCC/Clang/MSVC）上通常是 **16**（大小也是 16）。因此 `alignas(64)`、`alignas(32)` 这类要求都属于超对齐，标准库默认路径并不保证满足。
2. C++14 的 `new` 表达式只有一对 `operator new(std::size_t)` / `operator new(std::size_t, const std::nothrow_t&)`，**没有任何对齐参数**，实现只需保证返回地址适合 `max_align_t`。
所以 `new alignas(64) CacheLine` 得到的地址只保证 16 字节对齐，`alignas(64)` 在分配层面被**忽略**——类型本身仍是 64 对齐的（sizeof/成员布局按 64 对齐），但实际对象的起始地址不能保证是 64 的倍数。
3. 常见场景：
   - **cache line 对齐**（64 字节）：让高频写的计数器各自独占一条 cache line，消除 false sharing。
   - **SIMD**：AVX 需要 32 字节对齐、AVX-512 需要 64 字节，`_mm256_load_ps` 这类对齐加载指令否则会崩溃。
   - **DMA / 网卡与 RDMA 缓冲区**：硬件要求按页或按特定边界对齐，否则传输失败或降级。
   - **大页 / 页对齐**：减少 TLB miss。
   - **原子操作**：某些平台的 16 字节原子（CMPXCHG16B）要求 16 字节对齐。
4. C++14 只能手工处理，典型做法是「多分配 + 指针回退 + 记录原指针」：
```cpp
void* raw = ::operator new(sizeof(T) + alignment - 1 + sizeof(void*));
std::uintptr_t base = reinterpret_cast<std::uintptr_t>(raw) + sizeof(void*);
std::uintptr_t aligned = (base + alignment - 1) & ~(alignment - 1);
// 在 aligned - sizeof(void*) 处存回 raw，供释放时用
```
或者用平台 API：`posix_memalign` / `_aligned_malloc` / C11 `aligned_alloc`，再 placement new 构造、`p->~T()` + 专用释放函数析构。
缺点：**冗长且易错**（偏移、记录原指针、配对释放都要手写）、**不跨平台**（POSIX/Windows/不同编译器 API 不同）、每个类型都要重复一遍、无法与 `std::vector` 等容器的默认分配路径自动协作，`delete` 还容易写错成普通 delete。
5. 后果：`alignas(64)` 形同虚设 → 实际未对齐。
   - 相邻对象共享 cache line → **false sharing**，多核写同一条 line 导致性能断崖；
   - 对齐加载/存储指令（`_mm256_load_ps`、`movaps`）触发 **SIGSEGV/崩溃**；
   - DMA 传输失败或降级为慢路径；
   - 依赖对齐的原子/无锁结构可能出现撕裂或非原子行为；
   - 更麻烦的是它可能「碰巧对齐」从而长期不暴露，换编译器或换分配器才爆雷。

</details>
