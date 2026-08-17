# 约束与包含关系

## 原子约束

```cpp
// 原子约束：不可再分的约束
template <typename T>
concept A = std::integral<T>;  // 原子约束：integral<T>

// 合取（AND）
template <typename T>
concept B = std::integral<T> && std::signed_integral<T>;
// 两个原子约束：integral<T> 和 signed_integral<T>

// 析取（OR）
template <typename T>
concept C = std::integral<T> || std::floating_point<T>;
// 两个原子约束
```

## 包含关系（Subsumption）

```cpp
// Concept A 的约束比 B 更严格
template <typename T>
concept B = std::integral<T>;

template <typename T>
concept A = std::integral<T> && std::signed_integral<T>;
// A 包含 B 的所有约束 + 更多 → A 比 B 更严格

// 重载分派：更严格的 Concept 优先
void foo(B auto x) { std::cout << "integral"; }
void foo(A auto x) { std::cout << "signed integral"; }

foo(42);   // signed integral（A 更严格，优先）
foo(42u);  // integral（unsigned 不满足 A，走 B）
```

## 包含规则

```
如果 Concept P 的约束集是 Concept Q 的约束集的超集，
则 P subsumes Q（P 包含 Q），P 更严格。

重载分派时，编译器选择被 subsume 的（更严格的）Concept。
```

## 实际应用

```cpp
// 层次化 Concept
template <typename T> concept Range = requires(T r) { r.begin(); r.end(); };
template <typename T> concept SizedRange = Range<T> && requires(T r) { r.size(); };
template <typename T> concept RandomAccessRange = SizedRange<T> && requires(T r) {
    r[0];
};

// 重载：更严格的 Concept 优先
void process(Range auto& r) { /* 通用范围处理 */ }
void process(SizedRange auto& r) { /* 有 size() 的优化处理 */ }
void process(RandomAccessRange auto& r) { /* 随机访问最快 */ }

std::vector<int> v;
process(v);  // RandomAccessRange（最严格）

std::forward_list<int> l;
process(l);  // Range（最宽松）
```

## 注意事项

```cpp
// subsumption 只对原子约束有效
// 以下两个 Concept 不构成 subsumption：
template <typename T>
concept Even = (sizeof(T) % 2 == 0);  // 原子约束

template <typename T>
concept EvenAndIntegral = (sizeof(T) % 2 == 0) && std::integral<T>;
// EvenAndIntegral 不 subsume Even！
// 因为 (sizeof(T) % 2 == 0) 是表达式，不是原子约束
// 编译器不能确定它们是"同一个"约束

// 正确做法：用 Concept 组合
template <typename T>
concept EvenAndIntegral = Even<T> && std::integral<T>;
// 现在 EvenAndIntegral subsumes Even
```

## 自测题

1. 什么是原子约束？
2. Subsumption（包含关系）是什么？对重载分派有什么影响？
3. 更严格的 Concept 在重载中优先级如何？
4. 为什么表达式约束不构成 subsumption？
5. 设计 Range → SizedRange → RandomAccessRange 的层次化 Concept。
