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

<details>
<summary>参考答案</summary>

1. C++17 在 `<functional>` 中提供三种搜索器：`std::default_searcher`（朴素/实现默认，无预处理）、`std::boyer_moore_searcher`（坏字符表 + 好后缀表）、`std::boyer_moore_horspool_searcher`（只保留坏字符表）。
需要强调：**C++17 标准并未规定它们的复杂度与具体算法**，只要求搜索结果与 `std::search` 一致（返回最靠前的匹配）。教科书给出的典型量级是：预处理 O(m + σ)（σ 为字符集大小，用稀疏表可降到 O(m)），Boyer-Moore 系列平均约 O(n/m)、最坏 O(n·m)，`default_searcher` 为 O(n·m)。
2. 因为 Boyer-Moore 从**模式右端**开始比对：一次不匹配就能拿到「文本中这个字符」这条信息，配合坏字符表可以一次跳过最多约 m 个位置，于是平均只需约 n/m 次比对就滑过整个文本。
这是教科书在随机文本假设下给出的**平均**结论，最坏情况（如文本与模式都是 `aaaa...`）仍退化为 O(n·m)，标准也不作保证。
3. 模式很短（1～3 个字符）时用 `default_searcher`：预处理要建表（O(m + σ)），而短模式的最大跳跃距离不超过 m，跳跃收益极小，抵不过建表开销。
另外「只搜一次、文本也很短」的场景同样不值得预处理。只有在文本长、模式长或要搜多次时 Boyer-Moore 才划算。
4. 搜索器对象在**构造时**完成预处理（坏字符表、好后缀表），这部分成本是 O(m + σ) 且只与模式有关。
构造一次、对多份文本反复调用 `std::search(doc.begin(), doc.end(), searcher)`，预处理成本被摊薄，整体才划算；反过来每次搜索都现场构造搜索器，等于把预处理成本乘以搜索次数，反而更慢。
5. `boyer_moore_horspool_searcher` 只维护坏字符表，表更小、每步常数开销更低，**实践中通常更快，是默认首选**。
`boyer_moore_searcher` 额外维护好后缀表，在**长模式**、模式中重复结构多、或文本使坏字符规则频繁失效的场景下跳跃更大，收益更明显。
经验法则：短模式（1～3）→ `default_searcher`；中长模式 → `boyer_moore_horspool_searcher`；长模式且追求极致 → 两种都实测，选快的那个。

</details>
