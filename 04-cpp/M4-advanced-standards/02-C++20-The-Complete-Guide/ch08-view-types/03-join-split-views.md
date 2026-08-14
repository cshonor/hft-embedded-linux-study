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
