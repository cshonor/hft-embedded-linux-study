# 第 30 章 超对齐 new/delete

**new and delete with Over-Aligned Data**

## 本章讲什么

C++17 修复了一个长期缺陷：`new` 之前**不保证**按 `alignas` 指定的对齐分配内存。C++17 让 `new` 尊重超对齐（over-alignment，超过 `max_align_t` 的对齐，如 64 字节 cache 行）。

## 要点

### 问题背景

```cpp
struct alignas(64) CacheLine {  // 64 字节对齐（超对齐）
    int data[16];
};

// C++14：new 不保证 64 对齐！可能返回 16 对齐的指针
CacheLine* p = new CacheLine;   // 可能不对齐 64

// C++17：new 尊重 alignas，保证 64 对齐
CacheLine* p2 = new CacheLine;  // 一定 64 对齐
```

C++14 的 `operator new` 只保证 `max_align_t`（通常 8 或 16）对齐，`alignas(64)` 的类型用 `new` 可能不对齐——导致 cache 行跨界、SIMD 对齐访问崩溃。

### C++17 的修复

- `new T` 自动调用对齐感知的 `operator new(size, alignval)`。
- `delete p` 调用对应的 `operator delete(size, alignval)`。
- 自定义 `operator new` 要同时提供对齐版重载。

### 显式对齐分配

```cpp
// C++17 aligned_alloc（C11 兼容）
void* p = std::aligned_alloc(64, sizeof(Obj) * 10);  // 64 对齐，10 个 Obj
std::free(p);

// 或用 new 的对齐版
auto* p2 = new (std::align_val_t{64}) Obj;
delete p2;
```

### `std::aligned_alloc`（C++17）

```cpp
void* aligned_alloc(size_t alignment, size_t size);
// 要求 alignment 是 2 的幂，size 是 alignment 的倍数（某些平台）
```

### 影响自定义分配器

```cpp
// C++17 自定义 operator new 要加对齐重载
void* operator new(std::size_t size, std::align_val_t align);
void operator delete(void* p, std::size_t size, std::align_val_t align);
```

## HFT 关联

- **cache 行对齐结构**：`struct alignas(64) HotData {...};` 保证 `new HotData` 在 cache 行边界，避免伪共享。
- **SIMD 对齐**：AVX-512 要求 64 字节对齐，`alignas(64)` + `new` 保证加载不崩溃。
- **mempool 对齐**：自定义 mempool 分配时要考虑对齐——`allocate(n, align)` 传递对齐值。
- **C++17 前的 workaround**：HFT 代码以前用 `posix_memalign`/`_aligned_malloc` 手动对齐分配，C++17 后直接 `new`。
- **DPDK 对齐**：DPDK 的 `rte_malloc` 自带对齐参数，C++17 的对齐 new 与之语义对齐。

## 自测题

1. C++17 之前 `new` 对 `alignas(64)` 的类型有什么问题？
2. C++17 如何修复？`operator new` 有什么新重载？
3. `std::aligned_alloc` 的参数要求是什么？
4. HFT 为什么需要 cache 行对齐的 `new`？
5. 自定义 `operator new` 在 C++17 要加什么？
