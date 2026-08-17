# Item 10-11：了解分配器的限制与自定义用法

> 第 1 章 容器 · Item 10-11 · 上一节：[Item 9 删除元素的正确方式](item09-correct-element-removal.md)

## 为什么要学这个（先建立直觉）

C 程序员直接控制内存来源：

```c
// 从堆分配
void* p = malloc(64);
free(p);

// 从自定义内存池分配
void* p = pool_alloc(&my_pool, 64);
pool_free(&my_pool, p);

// 从 mmap 分配（hugepage）
void* p = mmap(NULL, 4096, PROT_READ|PROT_WRITE, MAP_HUGETLB, -1, 0);
munmap(p, 4096);
```

C++ STL 的容器默认用 `operator new`/`operator delete`（等价于 `malloc`/`free`）。如果想让容器从内存池或 hugepage 分配，就需要**自定义分配器**（allocator）。

```cpp
// 默认：用 operator new
std::vector<int> v;

// 自定义：用内存池
std::vector<int, MempoolAlloc<int>> v;
```

---

## 这节讲什么

分配器（allocator）自定义容器内存来源。默认 `std::allocator` 走 `operator new`。HFT 用自定义分配器接 mempool/hugepage。分配器有"相等性"约束——C++11 起要求无状态分配器才安全。

---

## 分配器基础

```cpp
// std::allocator 的简化模型
template<typename T>
class allocator {
public:
    T* allocate(size_t n) {
        return static_cast<T*>(::operator new(n * sizeof(T)));
    }
    void deallocate(T* p, size_t n) {
        ::operator delete(p);
    }
};

// std::vector 的第二个模板参数就是分配器
template<typename T, typename Alloc = std::allocator<T>>
class vector;
```

### 自定义分配器示例

```cpp
template<typename T>
class MempoolAlloc {
    Pool* pool_;
public:
    using value_type = T;
    explicit MempoolAlloc(Pool* p) : pool_(p) {}

    T* allocate(size_t n) {
        return static_cast<T*>(pool_alloc(pool_, n * sizeof(T)));
    }
    void deallocate(T* p, size_t) {
        pool_free(pool_, p);
    }
};

// 使用
Pool* pool = pool_create(1024 * 1024);  // 1MB 池
std::vector<int, MempoolAlloc<int>> v(MempoolAlloc<int>(pool));
v.push_back(42);  // 从内存池分配，不走 operator new
```

### 无状态 vs 有状态分配器

```cpp
// 无状态分配器（C++11 推荐）
template<typename T>
class StackAlloc {
public:
    using value_type = T;
    T* allocate(size_t n) { return static_cast<T*>(::operator new(n*sizeof(T))); }
    void deallocate(T* p, size_t) { ::operator delete(p); }
    // 无成员变量 → 无状态 → 所有实例"相等" → 可跨容器互换
};
// 比较运算符：总是返回 true
template<typename T, typename U>
bool operator==(const StackAlloc<T>&, const StackAlloc<U>&) { return true; }

// 有状态分配器（携带 pool 指针）
template<typename T>
class PoolAlloc {
    Pool* pool_;  // 有状态！
    // ...
};
// 两个 PoolAlloc 只有 pool_ 相同时才"相等"
```

---

## 常见错误（新手踩坑）

### 错误 1：有状态分配器跨容器 splice

```cpp
std::list<int, PoolAlloc<int>> a(PoolAlloc<int>(pool1));
std::list<int, PoolAlloc<int>> b(PoolAlloc<int>(pool2));
a.splice(a.end(), b);  // 如果 pool1 != pool2 → UB！
// b 的节点是 pool2 分配的，splice 到 a 后由 pool1 释放 → 错误的 pool free
```

**修正：** 确保 splice 的容器用相同 pool（分配器"相等"），或用无状态分配器。

### 错误 2：分配器类型不匹配导致无法赋值

```cpp
std::vector<int, MempoolAlloc<int>> v1(MempoolAlloc<int>(pool));
std::vector<int> v2;  // 默认 allocator
// v1 = v2;  // ❌ 类型不同（分配器不同）
```

**修正：** 统一分配器类型，或逐元素拷贝。

### 错误 3：忘了分配器是按值存储的

```cpp
Pool* pool = pool_create(1024);
{
    std::vector<int, PoolAlloc<int>> v(PoolAlloc<int>(pool));
    v.push_back(42);
}  // v 析构 → PoolAlloc deallocate → pool_free(pool, ...)
   // pool 仍然存活 ✅

pool_destroy(pool);  // 在所有容器销毁后才能销毁 pool
```

**修正：** 确保 pool 的生命周期长于所有使用它的容器。

---

## 新手要点（和 C 的区别）

| 维度 | C | C++ STL | 为什么 |
|------|---|---------|--------|
| 内存来源 | 直接调 malloc | 分配器抽象 | 可替换 |
| 自定义池 | 直接调 pool_alloc | 自定义 allocator | 类型安全 |
| 生命周期 | 手动管理 | 容器析构时释放 | RAII |
| 状态 | 无 | 无状态/有状态 | C++11 偏好无状态 |

**一句话：** C 直接调 `malloc`/`pool_alloc`，C++ STL 通过分配器抽象内存来源。自定义分配器让容器走 mempool/hugepage，但要注意有状态分配器的相等性约束。

---

## HFT 关联

- **mempool 分配器**：高频分配的小对象（如订单节点）用 `std::list<T, MempoolAlloc<T>>` 接 mempool，避免 `operator new` 的锁与碎片。
- **hugepage 分配器**：大缓冲区用 hugepage 分配器，减少 TLB miss，降低延迟尾部长尾。
- **无状态分配器优先**：C++11 推荐无状态分配器，避免 splice/swap 时的相等性问题。有状态分配器需要仔细管理 pool 生命周期。

---

## 代码自测

### Q1: 分配器的作用
```cpp
std::vector<int> v1;                                    // A
std::vector<int, MempoolAlloc<int>> v2(alloc);          // B
```
> A 和 B 在内存分配上有什么区别？

<details>
<summary>答案</summary>

- **A**：用默认 `std::allocator<int>`，内部走 `operator new`（等价于 `malloc`）。
- **B**：用自定义 `MempoolAlloc<int>`，从 `alloc` 指定的内存池分配。

自定义分配器让容器的内存来源可控——HFT 可以让 vector 从预分配的 mempool 取内存，避免热路径 `malloc` 的锁竞争。
</details>

### Q2: 无状态分配器
```cpp
template<typename T>
struct SimpleAlloc {
    using value_type = T;
    T* allocate(size_t n) { return (T*)::operator new(n*sizeof(T)); }
    void deallocate(T* p, size_t) { ::operator delete(p); }
};
bool operator==(const SimpleAlloc<T>&, const SimpleAlloc<U>&) { return true; }
```
> 为什么说这个分配器是"无状态"的？有什么好处？

<details>
<summary>答案</summary>

**无状态**：分配器没有成员变量（没有 `pool_` 指针等），所有实例等价（`operator==` 总是返回 true）。

**好处**：
1. 两个用相同无状态分配器的容器可以安全 `splice`/`swap`——分配器总是"相等"。
2. 不需要在容器间传递分配器状态。
3. C++11 标准推荐的无状态分配器模型。
</details>

### Q3: 有状态分配器的陷阱
```cpp
Pool* pool1 = pool_create(1024);
Pool* pool2 = pool_create(1024);

std::list<int, PoolAlloc<int>> a(PoolAlloc<int>(pool1));
std::list<int, PoolAlloc<int>> b(PoolAlloc<int>(pool2));
a.splice(a.end(), b);  // 安全吗？
```

<details>
<summary>答案</summary>

**不安全**（UB）。`a` 和 `b` 用不同的 pool，分配器"不相等"（`pool1 != pool2`）。splice 后 `b` 的节点被链入 `a`，但节点是 `pool2` 分配的。当 `a` 析构时用 `pool1` 释放这些节点 → 错误的 pool free。

**修正：** 确保 splice 的容器用相同的 pool，或用无状态分配器。
</details>

### Q4: pool 生命周期
```cpp
Pool* pool = pool_create(1024);
std::vector<int, PoolAlloc<int>>* v = new std::vector<int, PoolAlloc<int>>(PoolAlloc<int>(pool));
v->push_back(42);
pool_destroy(pool);  // A
delete v;             // B
```
> A 行在 B 行之前执行，有什么问题？

<details>
<summary>答案</summary>

**UB**。`pool_destroy(pool)` 销毁了 pool，但 `v` 还在用 `PoolAlloc` 从 pool 分配的内存。`delete v` 时 vector 析构 → `PoolAlloc::deallocate` → `pool_free(已销毁的 pool, ...)` → UB。

**修正：** 先 `delete v`（容器先析构，释放所有内存回 pool），再 `pool_destroy(pool)`。

pool 的生命周期必须长于所有使用它的容器。
</details>

---

## 参考与延伸

- 上一节：[Item 9 删除元素的正确方式](item09-correct-element-removal.md)
- 回到：[第 1 章 容器](README.md)
