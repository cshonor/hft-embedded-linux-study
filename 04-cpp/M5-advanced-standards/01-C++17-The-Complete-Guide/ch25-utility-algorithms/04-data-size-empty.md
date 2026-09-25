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

<details>
<summary>参考答案</summary>

1. 两个版本靠重载分开：对容器（以及任何有 `size()` 成员的类型）转发到成员函数，对内置数组则用模板推导出的数组长度。
```cpp
template <typename C>
constexpr auto size(const C& c) -> decltype(c.size()) { return c.size(); }

template <typename T, std::size_t N>
constexpr std::size_t size(const T (&/*array*/)[N]) noexcept { return N; }
```
`data`、`empty` 同理：`empty(c)` 转发 `c.empty()`，`empty(array)` 恒为 `false`。
2. 因为内置数组没有 `.size()` / `.data()` / `.empty()` 成员，泛型代码想同时接受 `int[10]` 和 `std::vector<int>` 就必须写两套分支。
非成员版本把这种差异收进标准库：一份模板代码就能统一处理 C 数组、`std::array`、`std::vector`、以及任何提供了这些成员函数的自定义容器，无需 `sizeof(arr)/sizeof(arr[0])` 这类易错写法，也不必为容器和数组各写一个重载。
3. `std::data(arr)` 返回指向首元素的指针（数组退化为 `T*`）；`std::data(v)` 返回 `v.data()`，即 `int*`。
```cpp
int arr[] = {1, 2, 3};
auto* p1 = std::data(arr);          // int*
std::vector<int> v{4, 5, 6};
auto* p2 = std::data(v);            // int*
const std::vector<int> cv{1};
auto* p3 = std::data(cv);           // const int*
```
对 const 容器返回 `const T*`，因此也能用于只读访问底层缓冲区。
4. 是。数组版 `std::size` 是 `constexpr`（长度在编译期已知），可以直接用在常量表达式里：
```cpp
int arr[10];
constexpr std::size_t n = std::size(arr);   // 10
static_assert(std::size(arr) == 10);
```
容器版则转发 `c.size()`，是否能在编译期求值取决于该容器的 `size()` 是否 `constexpr`（如 `std::array::size()` 是）。
5. 用非成员 `data`/`size`/`empty` 代替成员调用，同一份代码即可同时覆盖数组与容器：
```cpp
template <typename T>
auto sum(const T& container) {
    using Elem = std::remove_reference_t<decltype(*std::data(container))>;
    Elem s{};
    if (std::empty(container)) return s;
    for (std::size_t i = 0; i < std::size(container); ++i)
        s += std::data(container)[i];
    return s;
}
```
`sum(arr)`、`sum(v)`、`sum(a)` 都能通过编译——`std::data` 拿到首地址、`std::size` 拿到长度、`std::empty` 做空判断，无需为数组特化。

</details>
