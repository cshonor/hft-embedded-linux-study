# 3.4 编译期分派

> 第 3 章 迭代器与 traits · 第 4 节 · 上一节：[3.3 traits 萃取机制](03-traits-mechanism.md) · 下一节：[第 4 章 序列容器](../ch04-sequence-containers/README.md)

## 为什么要学这个（先建立直觉）

在 C 里，如果你想对不同数据结构用不同算法实现，只能用 `if` + 运行时判断，或用函数指针——都有运行时开销。STL 用 tag dispatch 在编译期选最优实现，零运行时开销。

```c
/* C: 运行时判断选算法 */
void advance(void* it, int n, int is_random_access) {
    if (is_random_access) {
        *(char**)it += n;  // 随机访问：一步
    } else {
        while (n--) (*(char**)it)++;  // 只能逐步
    }
    // 运行时 if 分支预测开销
}
```

```cpp
// C++ STL: 编译期 tag dispatch，零运行时开销
template<typename Iter, typename Dist>
void advance(Iter& it, Dist n) {
    using cat = typename iterator_traits<Iter>::iterator_category;
    advance_impl(it, n, cat{});  // 编译期选择重载
}
// vector: cat = random_access_iterator_tag → it += n (O(1))
// list: cat = bidirectional_iterator_tag → while(n--) ++it 或 --it
// 编译期已决定走哪个分支，运行时无 if
```

**直觉**：用类型标签（tag）做函数重载，编译器在编译期选最匹配的重载版本。不同迭代器分类编译出不同代码，零运行时分派开销。

## 这节讲什么

### tag dispatch 机制

```cpp
// 1. 定义标签（已有继承层级）
// input_iterator_tag → forward_iterator_tag → bidirectional_iterator_tag → random_access_iterator_tag

// 2. 为每种标签写不同实现
template<typename Iter, typename Dist>
void advance_impl(Iter& it, Dist n, std::input_iterator_tag) {
    while (n--) ++it;  // 只能前进，O(n)
}

template<typename Iter, typename Dist>
void advance_impl(Iter& it, Dist n, std::bidirectional_iterator_tag) {
    if (n >= 0) while (n--) ++it;
    else while (n++) --it;  // 可后退
}

template<typename Iter, typename Dist>
void advance_impl(Iter& it, Dist n, std::random_access_iterator_tag) {
    it += n;  // 一步到位，O(1)
}

// 3. 统一入口：用 traits 萃取 category，传 tag 对象
template<typename Iter, typename Dist>
void advance(Iter& it, Dist n) {
    using cat = typename std::iterator_traits<Iter>::iterator_category;
    advance_impl(it, n, cat{});  // cat{} 创建 tag 对象，触发重载决议
}
```

### distance 的编译期分派

```cpp
// InputIterator: 逐步计数
template<typename Iter>
typename iterator_traits<Iter>::difference_type
distance_impl(Iter first, Iter last, std::input_iterator_tag) {
    typename iterator_traits<Iter>::difference_type n = 0;
    while (first++ != last) ++n;
    return n;
}

// RandomAccessIterator: 一步算出
template<typename Iter>
typename iterator_traits<Iter>::difference_type
distance_impl(Iter first, Iter last, std::random_access_iterator_tag) {
    return last - first;  // 指针减法
}

template<typename Iter>
auto distance(Iter first, Iter last) {
    using cat = typename iterator_traits<Iter>::iterator_category;
    return distance_impl(first, last, cat{});
}
```

### copy 的联合分派

`std::copy` 同时用 `iterator_category` + `is_trivially_copyable` 做联合编译期分派：

```
RandomAccess + trivially_copyable → memmove  (最快)
RandomAccess + 非 trivially_copyable → 逐元素赋值
Input + trivially_copyable → 逐元素 memcpy（小批量）
Input + 非 trivially_copyable → 逐元素 placement new
```

### C++17 if constexpr 替代

```cpp
// C++17: if constexpr 替代 tag dispatch
template<typename Iter, typename Dist>
void advance(Iter& it, Dist n) {
    using cat = typename std::iterator_traits<Iter>::iterator_category;
    if constexpr (std::is_base_of_v<std::random_access_iterator_tag, cat>) {
        it += n;
    } else if constexpr (std::is_base_of_v<std::bidirectional_iterator_tag, cat>) {
        if (n >= 0) while (n--) ++it;
        else while (n++) --it;
    } else {
        while (n--) ++it;
    }
}
```

## 常见错误（新手踩坑）

### 错误 1：运行时 if 替代编译期分派

```cpp
// 反模式：运行时判断
template<typename Iter>
void advance(Iter& it, int n) {
    if (/* 怎么判断 Iter 是 RandomAccess？ */)  // 运行时无法判断类型！
        it += n;
    else
        while (n--) ++it;
}
// C++ 模板没有运行时类型信息（RTTI 不适用于此）
// 必须用 tag dispatch 或 if constexpr
```

### 错误 2：忘了传 tag 对象

```cpp
template<typename Iter, typename Dist>
void advance(Iter& it, Dist n) {
    using cat = typename iterator_traits<Iter>::iterator_category;
    advance_impl(it, n, cat);  // 错！cat 是类型，不是对象
    advance_impl(it, n, cat{});  // 对！cat{} 创建对象
}
```

### 错误 3：标签不匹配

```cpp
// 自定义迭代器声明了错误的 category
struct MyForwardIter {
    using iterator_category = std::random_access_iterator_tag;  // 声明随机访问
    // 但不支持 += 操作！
};
// std::sort(MyForwardIter{}, MyForwardIter{}) → 编译可能过，运行 UB
// 声明的 category 必须和实际能力一致
```

## 新手要点（和 C 的区别）

| 方面 | C | C++ STL |
|------|---|---------|
| 多态分派 | 函数指针（运行时） | tag dispatch（编译期） |
| 开销 | 间接调用 | 零（编译期内联） |
| 类型安全 | 无（void*） | 强类型（模板实例化） |
| 现代 C++ | — | if constexpr (C++17) / concepts (C++20) |

## HFT 关联

- **copy 的 memmove 特化**：编译期判断 trivially copyable + random_access → memmove，零运行时开销
- **编译期策略选择**：HFT 按数据类型在编译期选 SIMD/标量路径，避免运行时分支
- **if constexpr 替代 tag dispatch**：C++17 的 if constexpr 更直观，HFT 新代码优先使用

## 代码自测

### Q1: advance 分派

```cpp
std::vector<int> v = {1, 2, 3, 4, 5};
std::list<int> l = {1, 2, 3, 4, 5};

auto vit = v.begin();
std::advance(vit, 3);  // A

auto lit = l.begin();
std::advance(lit, 3);  // B
```
> A 和 B 分别走哪个 advance_impl？复杂度？

<details>
<summary>答案</summary>

- **A（vector::iterator = int*）**：category = `random_access_iterator_tag` → `advance_impl(..., random_access_iterator_tag{})` → `it += 3`，**O(1)**
- **B（list::iterator）**：category = `bidirectional_iterator_tag` → `advance_impl(..., bidirectional_iterator_tag{})` → `while(n--) ++it`，**O(n)**

编译期已决定走哪个版本，运行时无 if 判断。A 的汇编可能是 `lea rax, [rdi+12]`（一步），B 的汇编是循环。
</details>

### Q2: distance 分派

```cpp
auto d1 = std::distance(v.begin(), v.end());  // vector
auto d2 = std::distance(l.begin(), l.end());  // list
```
> d1 和 d2 的实现分别是什么？

<details>
<summary>答案</summary>

- **d1（vector）**：RandomAccess → `last - first`（指针减法），**O(1)**
- **d2（list）**：Input → 逐步 `++first` 计数，**O(n)**

这就是为什么 `vector::size()` 是 O(1)（`finish - start`）而 `list::size()` 在 C++11 前是 O(n)（需遍历计数）。

**HFT**：热路径需要 O(1) size 的容器用 vector/deque，不用 list/forward_list。
</details>

### Q3: if constexpr

```cpp
template<typename Iter>
void my_advance(Iter& it, int n) {
    if constexpr (std::is_same_v<typename std::iterator_traits<Iter>::iterator_category,
                                 std::random_access_iterator_tag>) {
        it += n;
    } else {
        while (n--) ++it;
    }
}
```
> 这和 tag dispatch 有什么区别？

<details>
<summary>答案</summary>

**功能等价**，都实现编译期分派。区别：

| 方面 | tag dispatch | if constexpr |
|------|-------------|---------------|
| 语法 | 函数重载 + tag 对象 | if + 类型判断 |
| 可读性 | 需要多个函数 | 单函数内分支 |
| 扩展性 | 加新 tag 只需加重载 | 加新分支需改原函数 |
| C++ 版本 | C++98 | C++17 |

**注意**：上面用 `is_same_v` 只精确匹配 RandomAccess，不匹配 Bidirectional。tag dispatch 利用继承——`random_access_iterator_tag` 继承 `bidirectional_iterator_tag`，传 RandomAccess 会匹配最具体的重载。`if constexpr` 要用 `is_base_of_v` 才等价：

```cpp
if constexpr (std::is_base_of_v<std::random_access_iterator_tag, cat>) {
    it += n;
} else if constexpr (std::is_base_of_v<std::bidirectional_iterator_tag, cat>) {
    // ...
}
```
</details>

### Q4: copy 的联合分派

```cpp
std::vector<int> src = {1, 2, 3, 4, 5};
std::vector<int> dst(5);

// A: int 是 trivially copyable + vector 是 RandomAccess
std::copy(src.begin(), src.end(), dst.begin());

// B: string 非 trivially copyable
std::vector<std::string> src2 = {"a", "b"};
std::vector<std::string> dst2(2);
std::copy(src2.begin(), src2.end(), dst2.begin());
```
> A 和 B 的 copy 分别走什么路径？

<details>
<summary>答案</summary>

- **A（int + RandomAccess）**：`is_trivially_copyable<int>` = true + RandomAccess → **memmove**（一次 memcpy，最快）
- **B（string + RandomAccess）**：`is_trivially_copyable<std::string>` = false → **逐元素赋值**（`dst[i] = src[i]`，调 operator=）

A 的汇编可能就是一条 `rep movsq`（x86 块拷贝指令），B 是循环调 `std::string::operator=`。

**HFT**：热路径数据结构设计为 trivially copyable（POD），让 `copy` 走 memmove 路径。避免 string/vector 等非 trivial 类型在拷贝时的逐元素开销。
</details>

## 参考与延伸

- 上一节：[3.3 traits 萃取机制](03-traits-mechanism.md)
- 下一节：[第 4 章 序列容器](../ch04-sequence-containers/README.md)
