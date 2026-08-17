# std::data / std::size / std::empty

## 统一接口

```cpp
#include <iterator>  // std::data
#include <vector>
#include <array>
#include <string>

int arr[5] = {1, 2, 3, 4, 5};
std::vector<int> v = {1, 2, 3};
std::string s = "hello";
std::array<double, 3> a = {1.0, 2.0, 3.0};

// std::size
std::size(arr);   // 5（数组）
std::size(v);     // 3（容器 .size()）
std::size(s);     // 5
std::size(a);     // 3

// std::data
int* p1 = std::data(arr);    // arr（数组退化为指针）
int* p2 = std::data(v);      // v.data()
char* p3 = std::data(s);     // s.data()
double* p4 = std::data(a);   // a.data()

// std::empty
std::empty(arr);   // false
std::empty(v);     // v.empty()
std::empty(s);     // s.empty()
```

## 为什么需要非成员版？

```cpp
// C 风格数组没有 .size()/.data()/.empty()
int arr[5];
arr.size();   // ❌ 编译错误
arr.data();   // ❌

// C++17 非成员版统一了接口
std::size(arr);   // 5
std::data(arr);   // arr
std::empty(arr);  // false

// 泛型代码不用区分数组和容器
template <typename T>
void process(T& container) {
    auto n = std::size(container);      // 数组或容器都行
    auto ptr = std::data(container);    // 数组或容器都行
    if (!std::empty(container)) {
        // ...
    }
}
```

## C 风格数组大小

```cpp
// C++14 之前：手写
int arr[10];
constexpr size_t n = sizeof(arr) / sizeof(arr[0]);  // 10

// C++17：std::size
constexpr size_t n = std::size(arr);  // 10

// std::size 对数组是 constexpr
static_assert(std::size(arr) == 10);
```

## 实际应用

```cpp
// 泛型函数：接受数组或容器
template <typename T>
auto sum(const T& container) {
    using Elem = std::remove_reference_t<
        decltype(*std::data(container))>;
    Elem s{};
    for (const auto& e : container) s += e;
    return s;
}

int arr[] = {1, 2, 3};
std::vector<int> v = {4, 5, 6};
std::array<int, 3> a = {7, 8, 9};

sum(arr);  // 6
sum(v);    // 15
sum(a);    // 24
```

## 自测题

1. `std::size` 对数组和容器分别怎么实现？
2. 为什么需要非成员版 `data`/`size`/`empty`？
3. `std::data(arr)` 返回什么？`std::data(v)` 呢？
4. `std::size` 对数组是 `constexpr` 吗？
5. 泛型代码中如何用这些函数统一处理数组和容器？
