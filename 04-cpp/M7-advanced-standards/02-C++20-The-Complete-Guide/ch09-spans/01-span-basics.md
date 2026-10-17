# std::span 基础

## 什么是 span

```cpp
#include <span>

// span：对连续内存的非拥有引用
// 类似 string_view 之于 string

int arr[5] = {1, 2, 3, 4, 5};
std::vector<int> v = {1, 2, 3, 4, 5};

// span 可以指向数组或 vector
std::span<int> s1(arr);       // 指向数组
std::span<int> s2(v);         // 指向 vector

// 函数参数：用 span 代替 (指针, 长度) 或模板
void process(std::span<int> data) {
    for (int x : data) { /* ... */ }
    data.size();   // 5
    data[0];       // 随机访问
    data.data();   // 底层指针
}

process(arr);
process(v);
```

## 固定大小 span

```cpp
// 动态大小（默认）
std::span<int> s1;  // 大小运行时确定

// 固定大小（编译期已知）
std::span<int, 5> s2(arr);  // 编译期保证 5 个元素
std::span<int, 3> s3(arr);  // 取前 3 个

void process5(std::span<int, 5> data) {
    // 编译期保证正好 5 个元素
    // 编译器可以更好地优化
}
```

## span 的优势

```cpp
// C++17：接受多种连续容器的函数
// 方式1：模板（太泛，什么类型都匹配）
template <typename T>
void process(T& container) { /* ... */ }

// 方式2：指针+长度（不安全，丢失大小信息）
void process(int* data, size_t n) { /* ... */ }

// 方式3：vector（不接受数组）
void process(std::vector<int>& v) { /* ... */ }

// C++20：span（统一、安全、高效）
void process(std::span<int> data) { /* ... */ }
// 接受数组、vector、array、string 等
// 有 size()、begin()/end()、operator[]
// 零拷贝（只是指针+大小）
```

## span 操作

```cpp
std::span<int> s(v);

s.size();        // 元素数
s.data();        // 底层指针
s.begin();       // 迭代器
s.end();
s[0];            // 随机访问
s.front();       // 第一个
s.back();        // 最后一个

// 子视图
s.first(3);      // 前 3 个
s.last(2);       // 后 2 个
s.subspan(1, 3); // 从位置 1 取 3 个

// span 不拥有数据——原始容器销毁后 span 悬空
```

## HFT 应用

```cpp
// 行情解析：span 接收原始缓冲
void parse_fix(std::span<const char> buf) {
    // buf.size() 知道长度
    // buf.data() 传给 C API
    // 零拷贝
}

char raw_buf[1024];
int n = recv(raw_buf, sizeof(raw_buf));
parse_fix(std::span(raw_buf, n));

// tick 数据处理
void process_ticks(std::span<const Tick> ticks) {
    for (const auto& t : ticks) {
        // 处理每条 tick
    }
}

std::vector<Tick> v;
process_ticks(v);  // 自动转 span
```

## 自测题

1. `std::span` 和指针+长度有什么区别？和 `vector` 呢？
2. 固定大小 span 和动态大小 span 的区别？
3. span 的 `first`/`last`/`subspan` 做什么？
4. span 拥有数据吗？什么情况下会悬空？
5. HFT 中 span 如何用于行情解析？

<details>
<summary>参考答案</summary>

1. `std::span<T>` 本质就是"**指针 + 长度**，外加一套容器接口"：它额外提供了 `size()`、`data()`、`begin()`/`end()`、`operator[]`、`front()`/`back()`、`first()`/`last()`/`subspan()`，因此可以直接用范围 for、传给 ranges 算法、被 `std::ranges::*` 识别。
另外它还能把长度写进**类型**（`span<T, N>` 静态长度），并用 `span<const T>` 表达只读语义——这是裸指针+长度表达不了的。
与 `vector` 的区别：`vector` **拥有**内存、可增删元素、自己分配释放；`span` **不拥有**，只是对已存在的一段连续内存的视图，大小不可变、拷贝是 O(1)、从不分配。把接口参数写成 `span<const T>` 就能同时接受 `vector`、`array`、C 数组和裸缓冲区，不必为每种容器写重载。
2. 模板第二个参数（extent）决定长度是编译期还是运行期：
   - **动态大小**：`std::span<T>`，等价于 `std::span<T, std::dynamic_extent>`（extent 为 `std::dynamic_extent`，即 `SIZE_MAX`）；长度运行时才知道，`size()` 是运行时值。
   - **固定大小**：`std::span<T, N>`（N > 0），长度写进类型，`extent` 为 N。它能从大小已知的数组直接构造（不必传长度），编译器知道长度，访问可更好地优化；长度不匹配的构造在编译期就失败。
两者可以互相转换（固定 → 动态总是可行），但动态 → 固定需要调用方保证长度。
3. 它们都返回**新的 span**（对同一段内存的视图），**不拷贝任何数据**：
   - `first(n)`：前 n 个元素；
   - `last(n)`：后 n 个元素；
   - `subspan(offset, count)`：从 offset 开始取 count 个（count 省略则取到末尾）。
固定 extent 的 span 还提供模板版本（如 `first<3>()`），长度在编译期检查。运行时版本的前提条件是 `n <= size()`、`offset <= size()`，越界属于未定义行为（不是抛异常）。
4. **不拥有数据**——它只持有指针和长度，是个"视图"。
悬空的典型情况：底层 `vector`/`string` 被销毁；`vector` 因 `push_back` 扩容而**重新分配**（旧迭代器/指针全部失效）；容器被 `clear()` 后重新填充；span 指向函数的**栈上局部数组**而函数已返回；或者 span 被保存到比底层缓冲更长的作用域。
规则很简单：**span 的生命周期必须严格短于它所引用的内存**，并且期间不能有使该内存失效的重分配。
5. 把"缓冲区 + 长度"直接作为 span 传给解析函数，全程零拷贝：
```cpp
void parse_fix(std::span<const char> buf);   // 只读视图

char raw_buf[1024];
int n = recv(raw_buf, sizeof(raw_buf));      // 收到 n 字节
parse_fix(std::span(raw_buf, n));            // 只包一层，不复制

void process_ticks(std::span<const Tick> ticks);
process_ticks(v);                            // vector 自动转成 span
```
好处：一份代码同时能吃 `vector<Tick>`、`std::array`、环形缓冲的一段；`span<const T>` 还强制了只读语义；传给 C API 时用 `data()` + `size()`。

</details>
