# 第 9 章 跨度 Spans

**Spans**

## 本章讲什么

`std::span<T>` 是 C++20 的"数组视图"——类似 `string_view` 之于 `string`，`span` 是**连续内存的非拥有视图**。替代 `T*` + `size_t` 的裸指针+长度组合。

## 要点

### 基本用法

```cpp
#include <span>

// span 参数：接受 vector、array、C 数组
void process(std::span<int> data) {
    for (int& x : data) { x *= 2; }    // 可遍历
    data.size();                        // 有 size
    data[0];                            // 有 []
    data.subspan(2, 3);                 // 子视图
}

std::vector<int> v = {1,2,3,4,5};
int arr[5] = {1,2,3,4,5};
std::array<int,5> a = {1,2,3,4,5};

process(v);      // vector → span
process(arr);    // C 数组 → span
process(a);      // array → span
process({v.data() + 1, 3});  // 指针+长度 → span
```

### 固定长度 span

```cpp
void foo(std::span<int, 3> s);   // 固定 3 个元素

int arr[3] = {1,2,3};
foo(arr);        // OK
foo(v);          // 编译错：v 长度未知（运行期）
```

固定长度 span（`span<T, N>`）编译期知道大小，更高效——无 size 字段，编译器优化更好。

### span 的特性

| 特性 | 说明 |
|------|------|
| 非拥有 | 不管理生命周期（像指针） |
| 连续内存 | 只支持连续存储（vector/array/C数组） |
| 可读可写 | `span<int>` 可改元素，`span<const int>` 只读 |
| 有 size | 不像裸指针要单独传长度 |
| 轻量 | 动态 span = 指针+size（16 字节）；固定 span = 指针（8 字节） |
| 可遍历 | 支持 begin/end/[] |

### span vs string_view vs 引用

| 类型 | 适用 | 拷贝开销 |
|------|------|----------|
| `span<T>` | 连续 T 数组 | 16 字节（动态）/ 8 字节（固定） |
| `string_view` | 字符串（const char 连续） | 16 字节 |
| `const vector<T>&` | vector 专用 | 引用（8 字节）但只接 vector |
| `T*` + `size_t` | C 风格 | 16 字节但无类型安全 |

span 统一了 vector/array/C 数组的接口——函数接受 `span<T>` 就能处理三种连续容器。

### 子视图

```cpp
std::span<int> s = v;
s.first(3);       // 前 3 个
s.last(2);        // 后 2 个
s.subspan(1, 3);  // 从位置 1 取 3 个
```

子视图零拷贝，只调整指针和长度。

## HFT 关联

- **行情缓冲处理**：`void parse(span<const char> buf)` 接受网络缓冲，零拷贝遍历。
- **统一 vector/array/C 数组接口**：策略函数用 `span<Tick>` 参数，接受 `vector<Tick>`/`array<Tick,N>`/`Tick[]` 三种实参。
- **固定长度 span 零开销**：`span<Tick, 10>` 编译期知道大小，无 size 字段，热路径最优。
- **替代 `T*` + `size`**：`parse(buf.data(), buf.size())` → `parse(buf)`，类型安全、少传参。
- **子视图切片**：`buf.subspan(header_size)` 跳过头部处理 payload，零拷贝。
- **`span<const T>` 只读**：行情快照传 `span<const Tick>` 确保不被修改。
- **不适用非连续容器**：`list`/`deque`/`map` 不是连续内存，不能用 span——用 Ranges。

## 自测题

1. `span` 和 `string_view` 的共同点和区别？
2. 动态 `span<T>` 和固定 `span<T, N>` 的区别？谁更高效？
3. `span` 能用于 `std::list` 吗？为什么？
4. `span<const T>` 和 `span<T>` 的区别？
5. HFT 行情缓冲处理为什么用 `span<const char>` 而非 `const char*` + `size_t`？
