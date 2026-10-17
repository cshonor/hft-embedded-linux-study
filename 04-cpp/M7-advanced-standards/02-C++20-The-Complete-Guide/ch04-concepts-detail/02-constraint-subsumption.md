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

<details>
<summary>参考答案</summary>

1. **原子约束**（atomic constraint）是约束表达式规范化后**不可再分解**的最小单位：一个表达式 `E` 加上它的参数映射（parameter mapping）。
合取（`&&`）和析取（`||`）**不是**原子约束——规范化时会把它们拆成左右两边的子约束。concept 名字会被展开成其定义中的约束，最终也落到原子约束上。
2. **Subsumption（包含）**：把两个约束规范化成析取范式的原子约束集合后，若 P 的约束集在语义上蕴含 Q 的（P 的每一项都能在 Q 里找到相同项），就说 **P subsumes Q**，即 P 更严格。
对重载分派的影响：当多个候选都可行、且其他方面（函数模板偏序等）打平时，编译器选**被 subsumes 的那个（更严格的）**——这是 C++20 特有的重载消解规则，也是 `enable_if` 时代做不到的。
3. **更严格的优先**。例子里 `Range` / `SizedRange` / `RandomAccessRange` 三个重载同时可行时，`std::vector<int>` 命中 `RandomAccessRange`（最严格），`std::forward_list<int>` 只能命中 `Range`（最宽松）。
前提：这些 concept 必须用**引用同一个 concept 的方式**层层组合（`SizedRange = Range<T> && requires{...}`），这样 subsumption 关系才成立。
4. 因为 subsumption 判定原子约束是否"相同"，依据的是**它们来自同一处源码表达式且参数映射相同**，而不是表达式算出来的值是否相等。
`Even` 里的 `(sizeof(T) % 2 == 0)` 和 `EvenAndIntegral` 里再写一遍的 `(sizeof(T) % 2 == 0)` 是**两个不同的源码表达式**，编译器不认为它们是同一个原子约束，于是 `EvenAndIntegral` 不 subsumes `Even`，重载就成了二义。
正确做法是把那个条件**包成一个 concept** 再引用：
```cpp
template <typename T> concept Even = (sizeof(T) % 2 == 0);
template <typename T> concept EvenAndIntegral = Even<T> && std::integral<T>;
```
这样 `Even` 作为一个原子约束确实出现在 `EvenAndIntegral` 的合取里，subsumption 成立。（同理，塞在 requires 表达式内部的约束也不会被拆出来参与 subsumption。）
5. 用**引用前一个 concept 的方式**层层加约束，形成 subsumption 链：
```cpp
template <typename T>
concept Range = requires(T r) { r.begin(); r.end(); };

template <typename T>
concept SizedRange = Range<T> && requires(T r) { r.size(); };

template <typename T>
concept RandomAccessRange = SizedRange<T> && requires(T r) { r[0]; };

void process(Range auto& r)              { /* 通用 */ }
void process(SizedRange auto& r)         { /* 有 size()，可预分配 */ }
void process(RandomAccessRange auto& r)  { /* 随机访问，最快 */ }
```
要点：每一层都**引用**上一层（`Range<T> && ...`）而不是把条件重新抄一遍，否则 subsumption 不成立、重载会二义。

</details>
