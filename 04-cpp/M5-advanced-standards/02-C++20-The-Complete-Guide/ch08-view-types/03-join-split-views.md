# join / split 视图

## join：展平嵌套范围

```cpp
std::vector<std::vector<int>> nested = {{1,2}, {3,4}, {5}};

// join：把嵌套范围展平
auto flat = nested | std::views::join;
// 1,2,3,4,5

// 字符串列表拼接
std::vector<std::string> words = {"hello", "world"};
auto chars = words | std::views::join;
// h,e,l,l,o,w,o,r,l,d（逐字符）
```

## split：按分隔符切割

```cpp
std::string s = "1,2,3,4,5";

// 按逗号分割
auto parts = s | std::views::split(',');
// 每个部分是一个子范围

for (auto part : s | std::views::split(',')) {
    std::string token(part.begin(), part.end());
    std::cout << token << '\n';
}
// 1
// 2
// 3
// 4
// 5
```

## split 的子范围

```cpp
// split 返回的是范围的范围
auto parts = s | std::views::split(',');
// parts 是一个范围，每个元素也是范围

// 需要遍历每个子范围
for (auto subrange : parts) {
    for (char c : subrange) {
        std::cout << c;
    }
    std::cout << '\n';
}

// C++23 的 lazy_split 和 split 区别
// split：前置分隔符的视图，适合双向/随机访问
// lazy_split：更通用的分割
```

## 字符串处理

```cpp
// 解析逗号分隔的数字
std::string input = "100,200,300,400";
std::vector<int> numbers;
for (auto part : input | std::views::split(',')) {
    std::string s(part.begin(), part.end());
    numbers.push_back(std::stoi(s));
}
// numbers = {100, 200, 300, 400}
```

## 其他实用视图

```cpp
// reverse：反转
auto rev = v | std::views::reverse;

// keys / values：map 的键/值
std::map<int, std::string> m = {{1,"a"}, {2,"b"}};
auto ks = m | std::views::keys;  // 1, 2
auto vs = m | std::views::values; // a, b

// elements<N>：取 tuple-like 的第 N 个元素
std::vector<std::pair<int, double>> pv = {{1,1.0}, {2,2.0}};
auto firsts = pv | std::views::elements<0>;  // 1, 2
```

## 自测题

1. `join` 做什么？能展平 `vector<vector<int>>` 吗？
2. `split` 的返回值是什么类型？
3. 如何用 `split` 解析逗号分隔字符串？
4. `keys` 和 `values` 视图做什么？
5. `elements<N>` 视图做什么？

<details>
<summary>参考答案</summary>

1. 它把"范围的范围"**展平一层**：遍历外层得到的每个内层范围，依次产出其元素。
```cpp
std::vector<std::vector<int>> nested = {{1,2}, {3,4}, {5}};
auto flat = nested | std::views::join;      // 1,2,3,4,5
```
能展平 `vector<vector<int>>`；`vector<string> | join` 则把每个字符串当成一个字符范围，得到逐字符序列（h,e,l,l,o,...）。
注意它**只展平一层**，且是惰性的——不构造中间容器，元素个数也不必事先知道。
2. 返回 `std::ranges::split_view`（`views::all_t<R>` 与分隔符视图的组合），是一个**视图**，其"元素"本身又是**子范围**（`subrange`）——所以它是"范围的范围"。
```cpp
auto parts = s | std::views::split(',');   // split_view
for (auto part : parts) { /* part 是一个子范围 */ }
```
按 C++20（P2210 之后）的语义，`split_view` 要求底层范围是 **forward_range 及以上**，其结果子范围是**不含分隔符**的相邻片段；C++23 另加了 `lazy_split_view` 来支持 input_range 的惰性分割。
3. ```cpp
std::string input = "100,200,300,400";
for (auto part : input | std::views::split(',')) {
    std::string tok(part.begin(), part.end());   // 子范围 → string
    int v = 0;
    std::from_chars(tok.data(), tok.data() + tok.size(), v);
}
```
因为切出来的是子范围而不是 `std::string`，需要时先构造字符串（或用 `std::string_view` 视图，注意其生命周期）。用 `from_chars` 而不是 `stoi` 可避免异常与 locale。
4. 它们作用于元素是 **pair-like / tuple-like** 的范围（典型是 `std::map` 的 `value_type`）：
   - `views::keys`：取每个元素的**第 0 个**成员（键）；
   - `views::values`：取每个元素的**第 1 个**成员（值）。
```cpp
std::map<int, std::string> m = {{1,"a"}, {2,"b"}};
auto ks = m | std::views::keys;     // 1, 2
auto vs = m | std::views::values;   // a, b
```
它们等价于 `views::elements<0>` 与 `views::elements<1>`。
5. 它取 tuple-like（如 `std::pair`、`std::tuple`、`std::array`）元素的**第 N 个成员**，是 `keys` / `values` 的泛化：
```cpp
std::vector<std::pair<int, double>> pv = {{1,1.0}, {2,2.0}};
auto firsts  = pv | std::views::elements<0>;   // 1, 2
auto seconds = pv | std::views::elements<1>;   // 1.0, 2.0
```
于是 `views::keys == views::elements<0>`、`views::values == views::elements<1>`。

</details>
