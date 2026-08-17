# 第 24 章 子串与子序列搜索器

**Substring and Subsequence Searchers**

## 本章讲什么

C++17 给 `<algorithm>` 加了 `std::search` 的搜索器重载：Boyer-Moore、Boyer-Moore-Horspool 算法，以及子序列搜索 `std::search` 的新形式。让大文本搜索从 O(n*m) 降到 O(n+m)。

## 要点

### 默认 `std::search` 的问题

```cpp
// C++14：线性搜索，O(n*m) 最坏
auto it = std::search(text.begin(), text.end(),
                      pattern.begin(), pattern.end());
```

朴素算法最坏 O(n*m)，大文本+长模式时慢。

### C++17 搜索器

```cpp
#include <algorithm>
#include <functional>

// 1. 默认搜索器（等价旧行为）
auto it1 = std::search(text.begin(), text.end(),
    std::default_searcher(pattern.begin(), pattern.end()));

// 2. Boyer-Moore（预处理 O(m)，搜索 O(n+m)，最坏 O(n*m) 但平均快）
auto it2 = std::search(text.begin(), text.end(),
    std::boyer_moore_searcher(pattern.begin(), pattern.end()));

// 3. Boyer-Moore-Horspool（BM 简化版，常数因子更小，实践中常更快）
auto it3 = std::search(text.begin(), text.end(),
    std::boyer_moore_horspool_searcher(pattern.begin(), pattern.end()));
```

### 搜索器选择

| 搜索器 | 预处理 | 搜索 | 适用 |
|--------|--------|------|------|
| `default_searcher` | O(1) | O(n*m) | 短模式、简单场景 |
| `boyer_moore_searcher` | O(m) | O(n+m) 平均 | 长模式、大文本 |
| `boyer_moore_horspool_searcher` | O(m) | O(n+m) 平均，常数小 | 实践首选 |

Boyer-Moore 系列**从右向左**匹配模式，利用坏字符表和好后缀表跳过不可能匹配的位置，平均比朴素快数倍。

### 子序列搜索（C++17 增强）

```cpp
// search 仍只找连续子串
auto it = std::search(text.begin(), text.end(), pat.begin(), pat.end());

// 要找"子序列"（不连续）用其他方法
// C++17 未直接提供子序列搜索，要手写或用 std::ranges（C++20）
```

### 重复搜索：构造一次搜索器

```cpp
// 搜索器可复用：预处理一次，搜索多次
std::boyer_moore_horspool_searcher searcher(pattern.begin(), pattern.end());

for (auto& doc : documents) {
    auto it = std::search(doc.begin(), doc.end(), searcher);
    if (it != doc.end()) { /* found */ }
}
```

搜索器对象保存预处理表，构造一次后可对多个文本搜索——适合"同一模式搜索多文档"场景。

## HFT 关联

- **FIX 协议字段搜索**：FIX 消息中找特定 tag（如 `|55=AAPL|`）用 Boyer-Moore-Horspool，比朴素快。
- **日志关键字过滤**：海量日志中搜索错误关键字，预处理搜索器复用，O(n+m) 扫描。
- **重复搜索优化**：监控多个合约的相同关键字，构造一次搜索器，遍历多份行情文本。
- **短模式仍用 default**：单字符或 2-3 字节模式，预处理开销 > 收益，用 `default_searcher`。
- **不适用于二进制协议**：BM 算法基于字符跳跃，二进制协议用定长解析更快。

## 自测题

1. C++17 的三种搜索器分别是什么？复杂度对比？
2. Boyer-Moore 为什么比朴素搜索快？它的匹配方向是什么？
3. 搜索器为什么要"构造一次，搜索多次"？什么场景适用？
4. `boyer_moore_horspool` 相比 `boyer_moore` 的优势是什么？
5. HFT FIX 协议字段搜索为什么用 BMH 搜索器？短模式为什么不用？

## 代码自测

### Q1: 子串搜索
```cpp
std::string text = "The quick brown fox jumps";
std::string pattern = "brown";

// C++17: search 可用优化算法
auto it = std::search(text.begin(), text.end(),
    std::make_searcher(std::boyer_moore_searcher(pattern.begin(), pattern.end())));

if (it != text.end()) {
    std::cout << "Found at: " << (it - text.begin());
}
```
> Boyer-Moore 搜索算法比朴素搜索快在哪里？什么场景适合用？

<details>
<summary>答案与复习指引</summary>

**Boyer-Moore 算法**：
- 预处理模式串构建"坏字符表"和"好后缀表"
- 搜索时从模式末尾开始比较，不匹配时可以跳过多个字符
- 平均 O(n/m)（比朴素 O(n*m) 快），最坏 O(n*m)

**C++17 提供的搜索器**：
| 搜索器 | 预处理 | 搜索 | 适用场景 |
|--------|--------|------|---------|
| `default_searcher` | 无 | O(n*m) | 短模式/简单场景 |
| `boyer_moore_searcher` | O(m) | O(n/m) 平均 | 中等长度模式 |
| `boyer_moore_horspool_searcher` | O(m) | O(n/m) 平均 | 实践中常更快（简化版） |

**HFT**：协议解析中搜索定界符可用，但热路径通常用固定偏移/状态机，不搜索。

**复习：** → [子串搜索](./README.md)
</details>
