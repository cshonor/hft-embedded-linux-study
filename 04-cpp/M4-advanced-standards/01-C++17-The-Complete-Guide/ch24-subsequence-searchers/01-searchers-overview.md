# 搜索器概览

## C++17 前的 std::search

```cpp
// 朴素搜索：O(n*m) 最坏
auto it = std::search(text.begin(), text.end(),
                      pattern.begin(), pattern.end());
// 在 text 中找 pattern 的首次出现位置
```

朴素算法逐字符比较，最坏 O(n*m)（n = text 长度，m = pattern 长度）。

## C++17 搜索器

```cpp
#include <algorithm>
#include <functional>

// 1. default_searcher：等价旧行为
auto it1 = std::search(text.begin(), text.end(),
    std::default_searcher(pattern.begin(), pattern.end()));

// 2. boyer_moore_searcher
auto it2 = std::search(text.begin(), text.end(),
    std::boyer_moore_searcher(pattern.begin(), pattern.end()));

// 3. boyer_moore_horspool_searcher
auto it3 = std::search(text.begin(), text.end(),
    std::boyer_moore_horspool_searcher(pattern.begin(), pattern.end()));
```

## 复杂度对比

| 搜索器 | 预处理 | 搜索（平均） | 搜索（最坏） | 空间 |
|--------|--------|-------------|-------------|------|
| `default_searcher` | O(1) | O(n*m) | O(n*m) | O(1) |
| `boyer_moore_searcher` | O(m) | O(n/m) | O(n*m) | O(m + 字母表) |
| `boyer_moore_horspool_searcher` | O(m) | O(n/m) | O(n*m) | O(字母表) |

**关键**：Boyer-Moore 系列平均 O(n/m)——比线性还快！因为可以跳过不可能匹配的位置。

## 选择指南

```
短模式（1-3 字符）→ default_searcher（预处理开销不值）
中等模式（4-50）  → boyer_moore_horspool_searcher（常数因子小）
长模式（50+）     → boyer_moore_searcher（好后缀表收益大）
重复搜索同一模式  → 构造搜索器一次，搜索多次
```

## 搜索器是可复用对象

```cpp
// 构造一次搜索器（含预处理表）
std::boyer_moore_horspool_searcher searcher(pattern.begin(), pattern.end());

// 对多个文本搜索
for (auto& doc : documents) {
    auto it = std::search(doc.begin(), doc.end(), searcher);
    if (it != doc.end()) {
        // found
    }
}
// 预处理只做一次，多次搜索复用
```

## 自测题

1. C++17 的三种搜索器分别是什么？各自的复杂度？
2. Boyer-Moore 系列的平均搜索复杂度为什么是 O(n/m)？
3. 什么场景用 `default_searcher` 而不用 BM？
4. 搜索器为什么要"构造一次，搜索多次"？
5. `boyer_moore_searcher` 和 `boyer_moore_horspool_searcher` 怎么选？
