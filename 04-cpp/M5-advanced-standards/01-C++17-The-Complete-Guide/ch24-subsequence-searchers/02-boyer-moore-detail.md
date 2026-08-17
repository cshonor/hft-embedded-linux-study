# Boyer-Moore 算法原理

## 核心思想

Boyer-Moore 从**右向左**匹配模式，利用两个启发式跳过不可能匹配的位置：

1. **坏字符规则（Bad Character）**：不匹配时，根据文本中的字符在模式中的位置，跳过不可能匹配的位置。
2. **好后缀规则（Good Suffix）**：部分匹配后，利用已匹配的后缀信息跳过。

## 坏字符规则示例

```
文本：  ...A B C A B X...
模式：      A B C A B

从右向左比较：
B vs B ✓
A vs A ✓
B vs B ✓
C vs C ✓
A vs A ✓
X vs A ✗ ← 不匹配

X（坏字符）不在模式中 → 跳过整个模式长度 m=5
```

如果坏字符在模式中存在，则移动到对齐的位置。

## Horspool 简化

Boyer-Moore-Horspool 只用**坏字符规则**，去掉好后缀表：
- 预处理更简单（O(m) → O(字母表)）
- 常数因子更小
- 实践中通常更快

```
Horspool 总是用文本中与模式末尾对齐的字符来决定跳转距离
```

## 预处理：坏字符表

```cpp
// 简化的坏字符表构建（Horspool 风格）
std::array<size_t, 256> bad_char;  // ASCII
bad_char.fill(m);  // 默认跳 m（模式长度）

for (size_t i = 0; i < m - 1; ++i) {
    bad_char[static_cast<unsigned char>(pattern[i])] = m - 1 - i;
}
// pattern 中最后出现的字符位置决定跳转距离
```

## 搜索过程

```cpp
size_t i = 0;  // text 中的当前位置
while (i + m <= n) {
    size_t j = m - 1;  // 从模式末尾开始比较
    while (j < m && text[i + j] == pattern[j]) {
        --j;
    }
    if (j == SIZE_MAX) {
        // 找到匹配，位置 i
        return i;
    }
    // 跳转：根据 text[i + m - 1] 的坏字符表
    i += bad_char[static_cast<unsigned char>(text[i + m - 1])];
}
```

## 为什么从右向左？

从右向左比较让算法能"看到"更多的文本字符——如果末尾不匹配，整个模式可以跳过。从左向右只能跳 1 个位置（朴素算法）。

## 自测题

1. Boyer-Moore 的匹配方向是什么？为什么这样选？
2. 坏字符规则的原理是什么？
3. Horspool 简化了什么？为什么反而可能更快？
4. 坏字符表怎么构建？空间复杂度？
5. 为什么从右向左比从左向右能跳更多？
