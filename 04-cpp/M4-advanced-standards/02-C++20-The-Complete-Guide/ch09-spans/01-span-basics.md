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
