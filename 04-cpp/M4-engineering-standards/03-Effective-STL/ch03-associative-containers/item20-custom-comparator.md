# Item 20：为关联容器指定比较类型

> 第 3 章 关联容器 · Item 20 · 上一节：[Item 18-19 相等与等价](item18-19-equality-equivalence.md) · 下一节：[Item 21 map 键是 const](item21-map-key-is-const.md)

## 为什么要学这个（先建立直觉）

C 程序员用 `qsort` 时传比较函数：

```c
int cmp(const void* a, const void* b) {
    return *(const int*)a - *(const int*)b;
}
qsort(arr, n, sizeof(int), cmp);  // 传函数指针
```

C++ 的 `set`/`map` 不接受函数指针——它要求比较**类型**（一个有 `operator()` 的类）：

```cpp
struct PtrCmp {
    bool operator()(int* a, int* b) const { return *a < *b; }
};
std::set<int*, PtrCmp> s;  // 按指针所指值排序，而非指针地址
```

---

## 这节讲什么

`set<K>` 的第三个模板参数是比较**类型**而非函数指针。比较类型是无状态的函数对象（stateless functor）。Lambda 不能直接作类型参数，要用 `decltype` 或写 struct。

---

## 比较类型用法

```cpp
// 默认比较：std::less<T>（即 operator<）
std::set<int> s;  // 等价于 set<int, std::less<int>>

// 自定义比较类型
struct CaseInsensitiveCmp {
    bool operator()(const std::string& a, const std::string& b) const {
        return strcasecmp(a.c_str(), b.c_str()) < 0;
    }
};
std::set<std::string, CaseInsensitiveCmp> s;  // 不区分大小写排序

// 降序排列
std::set<int, std::greater<int>> desc;  // 从大到小

// 按指针所指值排序
struct PtrCmp {
    bool operator()(int* a, int* b) const { return *a < *b; }
};
std::set<int*, PtrCmp> ptr_set;
```

### Lambda 作比较类型（C++11）

```cpp
// Lambda 没有类型名，不能直接用作模板参数
// std::set<int, [](int a, int b){ return a > b; }> s;  // ❌

// 用 decltype + 构造函数传参
auto cmp = [](int a, int b) { return a > b; };
std::set<int, decltype(cmp)> s(cmp);  // 需要传 lambda 实例

// 更简单：直接写 struct
struct DescCmp {
    bool operator()(int a, int b) const { return a > b; }
};
std::set<int, DescCmp> s;  // 不需要传构造参数（无状态）
```

---

## 常见错误（新手踩坑）

### 错误 1：传函数指针而非比较类型

```cpp
bool cmp(int a, int b) { return a > b; }
// std::set<int, cmp> s;  // ❌ cmp 是函数，不是类型
std::set<int, bool(*)(int,int)> s(cmp);  // ⚠️ 可以但性能差（函数指针阻止内联）
```

**修正：** 用 struct 重载 `operator()`（函数对象），编译器可内联。

### 错误 2：有状态的比较类型

```cpp
struct BadCmp {
    int threshold;  // 有状态！
    BadCmp(int t) : threshold(t) {}
    bool operator()(int a, int b) const { return abs(a-threshold) < abs(b-threshold); }
};
// set<BadCmp> 的比较器有状态 → splice/swap 可能不安全
```

**修正：** 比较类型应无状态（没有非 const 成员变量）。如果需要参数化，用编译期常量或模板。

### 错误 3：比较函数不是严格弱序

```cpp
struct BadCmp {
    bool operator()(int a, int b) const { return a <= b; }  // ❌ 非严格弱序
};
// a <= b 且 b <= a → a 等价 a → 自反性违反 → set 行为未定义
```

**修正：** 比较函数必须是严格弱序（strict weak ordering）：非自反（`!(a<a)`）、非对称（`a<b` → `!(b<a)`）、传递（`a<b && b<c` → `a<c`）。

---

## 新手要点（和 C 的区别）

| 维度 | C `qsort` | C++ `set<K, Cmp>` | 为什么 |
|------|-----------|-------------------|--------|
| 比较方式 | 函数指针 | 比较类型（函数对象） | 可内联 |
| 传参 | 运行时传函数 | 编译期固定类型 | 类型安全 |
| 性能 | 间接调用 | 内联优化 | 零开销 |

**一句话：** C 的 `qsort` 传函数指针（间接调用，不可内联）。C++ 的关联容器用比较类型（函数对象），编译器可内联 `operator()`，零开销。

---

## HFT 关联

- **自定义比较排序订单**：订单簿按价格优先排序，用自定义比较类型 `PriceTimeCmp`，编译器内联比较逻辑。
- **lambda 比较 + decltype**：快速原型用 lambda + `decltype`，生产代码用 struct（更清晰、可复用）。
- **严格弱序检查**：自定义比较必须满足严格弱序，否则 `set`/`map` 行为未定义——HFT 系统不能容忍 UB。

---

## 代码自测

### Q1: 比较类型 vs 函数指针
```cpp
bool cmp(int a, int b) { return a > b; }
struct CmpType { bool operator()(int a, int b) const { return a > b; } };

// A
std::set<int, bool(*)(int,int)> s1(cmp);
// B
std::set<int, CmpType> s2;
```
> A 和 B 在性能上有什么区别？

<details>
<summary>答案</summary>

**B 更快**。A 用函数指针，每次比较是间接调用（不可内联）。B 用函数对象（functor），类型唯一且编译期已知，编译器可内联 `operator()`，零调用开销。

HFT 热路径的比较器用 struct/functor，不用函数指针。
</details>

### Q2: 降序 set
```cpp
std::set<int, std::greater<int>> s = {3, 1, 4, 1, 5};
for (auto x : s) std::cout << x << ' ';
```
> 输出什么？

<detailf>
<summary>答案</summary>

输出 `5 4 3 1`。`std::greater<int>` 让 set 降序排列。重复的 1 只保留一个。
</details>

### Q3: Lambda 比较
```cpp
auto cmp = [](int a, int b) { return a % 10 < b % 10; };
std::set<int, decltype(cmp)> s(cmp);
s.insert({23, 15, 47, 11, 33});
for (auto x : s) std::cout << x << ' ';
```
> 输出什么？

<detailf>
<summary>答案</summary>

输出 `11 23 33 15 47`。按个位数排序：1, 3, 3, 5, 7。注意 23 和 33 的个位都是 3 → 等价 → 只保留先插入的 23。

`decltype(cmp)` 获取 lambda 的类型，构造时传 `cmp` 实例。
</details>

### Q4: 严格弱序
```cpp
struct BadCmp {
    bool operator()(int a, int b) const { return a <= b; }  // 有问题
};
std::set<int, BadCmp> s;
s.insert(1);
s.insert(1);
```
> 会发生什么？

<detailf>
<summary>答案</summary>

**未定义行为**。`a <= b` 不是严格弱序——`1 <= 1` 为 true，违反非自反性（`!(a<a)`）。set 的内部红黑树不变量被破坏，行为未定义（可能崩溃、可能插入重复、可能死循环）。

**修正：** 用 `<`（严格小于），满足严格弱序。
</details>

---

## 参考与延伸

- 上一节：[Item 18-19 相等与等价](item18-19-equality-equivalence.md)
- 下一节：[Item 21 map 键是 const](item21-map-key-is-const.md)
- 回到：[第 3 章 关联容器](README.md)
