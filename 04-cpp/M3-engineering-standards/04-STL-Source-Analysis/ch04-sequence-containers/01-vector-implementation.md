# 4.1 vector 源码：三指针与扩容

> 第 4 章 序列容器 · 第 1 节 · 上一节：[本章概览](README.md) · 下一节：[4.2 list 源码](02-list-implementation.md)

## 为什么要学这个（先建立直觉）

在 C 里，动态数组要手写——`malloc` + `realloc` + 手动跟踪 size/capacity。vector 把这些封装成三指针模型，但理解源码才能避免性能陷阱。

```c
/* C: 手写动态数组 */
int* data = NULL;
size_t size = 0, cap = 0;
void push(int val) {
    if (size == cap) {
        cap = cap ? cap * 2 : 4;
        data = realloc(data, cap * sizeof(int));  // 可能搬迁！
    }
    data[size++] = val;
}
```

```cpp
// C++ vector: 三指针封装
std::vector<int> v;
v.push_back(42);
// 内部：start/finish/end_of_storage 三指针管理
// 扩容：分配 2x → 移动旧元素 → 释放旧内存
```

**直觉**：vector 本质就是 C 动态数组的 RAII 封装。三指针分别记录"已用头"、"已用尾"、"容量尾"。

## 这节讲什么

### 三指针模型

```cpp
template<typename T, typename Alloc = std::allocator<T>>
class vector {
    T* start;           // 已用空间起点
    T* finish;          // 已用空间末尾（size 边界）
    T* end_of_storage;  // 总分配空间末尾（capacity 边界）
public:
    size_t size() const { return finish - start; }
    size_t capacity() const { return end_of_storage - start; }
    bool empty() const { return start == finish; }
    T& operator[](size_t n) { return *(start + n); }
    T* begin() { return start; }
    T* end() { return finish; }
};
```

```
[start ---------- finish ---------- end_of_storage]
 |------ size ------|------ 剩余容量 ------|
```

### push_back 源码

```cpp
void push_back(const T& val) {
    if (finish != end_of_storage) {
        // capacity 够：原地构造，O(1)
        construct(finish, val);  // placement new: ::new(finish) T(val)
        ++finish;
    } else {
        // capacity 不够：扩容
        const size_t old_size = size();
        const size_t new_size = old_size ? 2 * old_size : 1;  // 翻倍
        T* new_start = alloc.allocate(new_size);  // 分配新内存
        T* new_finish = new_start;
        try {
            // 移动旧元素到新内存
            new_finish = std::uninitialized_copy(
                std::make_move_iterator(start),
                std::make_move_iterator(finish),
                new_start);
            // 构造新元素
            construct(new_finish, val);
            ++new_finish;
        } catch (...) {
            // 异常安全：析构已构造的，释放新内存
            destroy(new_start, new_finish);
            alloc.deallocate(new_start, new_size);
            throw;
        }
        // 析构旧元素，释放旧内存
        destroy(begin(), end());
        alloc.deallocate(start, end_of_storage - start);
        // 更新三指针
        start = new_start;
        finish = new_finish;
        end_of_storage = new_start + new_size;
    }
}
```

### 扩容因子

| 实现 | 扩容因子 | 特点 |
|------|---------|------|
| GCC libstdc++ | 2x | 简单，但可能浪费内存 |
| MSVC | 1.5x | 更省内存，且能复用旧内存 |
| Clang libc++ | 2x | 同 GCC |

**2x vs 1.5x**：1.5x 在多次扩容后，旧内存可以被新内存复用（因为 1+1.5 > 2.5 > 2）；2x 永远不能复用旧内存。

### 迭代器失效规则

```cpp
std::vector<int> v = {1, 2, 3};
auto it = v.begin();

v.push_back(4);  // 如果扩容 → it 失效！
// *it = 10;  // UB！

v.reserve(100);  // 确保不扩容
v.push_back(5);  // it 仍然有效
*it = 10;  // OK
```

**规则**：
- 扩容 → 所有迭代器/指针/引用失效
- `push_back`（未扩容）→ 仅 end() 变化，其他有效
- `erase` → 被删元素及之后的迭代器失效
- `insert`（未扩容）→ 插入点及之后的失效

## 常见错误（新手踩坑）

### 错误 1：循环中 push_back 不 reserve

```cpp
std::vector<int> v;
for (int i = 0; i < 1000000; i++) {
    v.push_back(i);  // 多次扩容 → 多次搬移 → O(n²) 总开销
}
// 修复
v.reserve(1000000);  // 一次分配，零扩容
```

### 错误 2：扩容后用旧迭代器

```cpp
auto it = v.begin();
v.push_back(42);  // 可能扩容
*it = 10;  // 可能 UB
```

### 错误 3：以为 capacity 会自动缩小

```cpp
v.reserve(1000000);
v.push_back(1);
v.shrink_to_fit();  // 请求缩小到 size
// 但 shrink_to_fit 是非绑定的——实现可以忽略
```

## 新手要点（和 C 的区别）

| 方面 | C (手写数组) | C++ vector |
|------|-------------|-----------|
| 扩容 | realloc（可能原地） | 分配新+移动+释放旧 |
| 内存管理 | 手动 free | RAII 自动释放 |
| 迭代器失效 | 指针失效（realloc） | 同（扩容时） |
| 类型安全 | void* | 模板强类型 |

## HFT 关联

- **reserve 消除热路径扩容**：启动时 reserve 到最大需求量，运行时零扩容
- **连续内存换 cache**：vector 数据连续，遍历 cache 命中率高，热路径首选
- **移动语义减少扩容开销**：C++11 后扩容用 `uninitialized_copy` + `make_move_iterator`，对有移动构造的类型零拷贝

## 代码自测

### Q1: 三指针

```cpp
std::vector<int> v;
v.reserve(5);     // start=0x100, finish=0x100, end_of_storage=0x114
v.push_back(1);   // finish=0x104
v.push_back(2);   // finish=0x108
v.push_back(3);   // finish=0x10C
// size() = ?  capacity() = ?
```

<details>
<summary>答案</summary>

- `size()` = `finish - start` = `0x10C - 0x100` = `0xC` = **3**
- `capacity()` = `end_of_storage - start` = `0x114 - 0x100` = `0x14` = **5**

还有 2 个空位（finish 到 end_of_storage）。

**HFT**：reserve 后 push_back 不触发扩容，O(1) 尾插。
</details>

### Q2: 扩容开销

```cpp
std::vector<int> v;
for (int i = 0; i < 1000; i++) v.push_back(i);
// 如果不 reserve，总共拷贝多少次元素？
```

<details>
<summary>答案</summary>

GCC 2x 扩容：1→2→4→8→16→32→64→128→256→512→1024

总拷贝次数 = 1 + 2 + 4 + 8 + ... + 512 ≈ 1023（几何级数和）

约 **1023 次拷贝**（加上 1000 次 push_back 本身 = 约 2023 次操作）。

如果 reserve(1000)：只有 1000 次 push_back，**零额外拷贝**。

**教训**：reserve 消除扩容开销。push_back N 个元素不 reserve 是 O(N) 总拷贝（均摊 O(1) 但有尖峰）；reserve 后是纯 O(N) 无尖峰。
</details>

### Q3: 迭代器失效

```cpp
std::vector<int> v = {1, 2, 3, 4, 5};
auto it = v.begin() + 2;  // 指向 3
v.erase(v.begin());       // 删除第一个元素
std::cout << *it;         // 安全吗？输出什么？
```

<details>
<summary>答案</summary>

**不安全**。`erase` 删除第一个元素后，后面的元素前移一位，`it` 指向的位置变成了原来的第 4 个元素（4），但标准说 erase 之后的迭代器失效。

虽然实际上 `*it` 可能输出 4（元素前移了），但这是 **UB**（未定义行为）——不同编译器/优化级别可能不同。

**修复**：erase 返回下一个有效迭代器。

```cpp
auto it = v.begin() + 2;
v.erase(v.begin());
it = v.begin() + 1;  // 重新获取（原来+2，删除1个后变成+1）
std::cout << *it;  // 安全，输出 3
```

**规则**：vector 的 erase 使被删元素及之后的所有迭代器失效。
</details>

### Q4: 移动语义扩容

```cpp
struct BigBuffer {
    int data[1024];
    BigBuffer(BigBuffer&& other) noexcept {
        memcpy(data, other.data, sizeof(data));
    }
    BigBuffer(const BigBuffer&) { /* 逐字节拷贝 4KB */ }
};
std::vector<BigBuffer> v;
v.push_back(BigBuffer{});  // 扩容时用移动还是拷贝？
```

<details>
<summary>答案</summary>

**用移动**（因为 `BigBuffer` 的移动构造是 `noexcept`）。

vector 扩容时检查元素类型的移动构造是否 `noexcept`：
- `noexcept` → 用 `uninitialized_copy` + `make_move_iterator`（移动语义，零拷贝）
- 非 `noexcept` → 用拷贝（保证强异常安全：移动到一半抛异常会损坏源对象）

```cpp
// 如果移动构造不是 noexcept
struct BadBuffer {
    int data[1024];
    BadBuffer(BadBuffer&&) { /* 可能抛异常 */ }  // non-noexcept
};
// vector 扩容时用拷贝（4KB × N 次），不是移动！
```

**HFT**：热路径类型定义 `noexcept` 移动构造，让 vector 扩容走移动语义避免拷贝。
</details>

## 参考与延伸

- 上一节：[本章概览](README.md)
- 下一节：[4.2 list 源码](02-list-implementation.md)
