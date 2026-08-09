# Item 38：仿函数按值传递

> 第 6 章 仿函数与函数 · Item 38 · 上一节：[ch05-algorithms](../ch05-algorithms/README.md) · 下一节：[Item 39 predicate 可配对](item39-predicate-adaptable.md)

## 为什么要学这个（先建立直觉）

在 C 里，你传给 `qsort` 的比较函数是函数指针——按值传递指针，函数本身不会被拷贝。C++ STL 的做法不同：它按**值**拷贝仿函数对象本身。

```c
/* C: 函数指针按值传递（只拷贝 8 字节指针） */
int cmp(const void *a, const void *b) { return *(int*)a - *(int*)b; }
qsort(arr, n, sizeof(int), cmp);  // cmp 的地址被传进去
```

```cpp
// C++: 仿函数对象本身被按值拷贝
struct Sum {
    int total = 0;
    void operator()(int x) { total += x; }  // 注意：non-const!
};
std::vector<int> v = {1, 2, 3};
Sum result = std::for_each(v.begin(), v.end(), Sum{});  // 拷贝传入，拷贝返回
std::cout << result.total;  // 6（但内部副本的状态不可预测）
```

**直觉**：STL 算法会拷贝你的仿函数（可能多次拷贝），所以它必须**轻量**且 `operator()` 应为 **const**。

## 这节讲什么

STL 算法（`for_each`、`transform`、`find_if`、`sort` 等）按值传递仿函数，意味着：

1. 仿函数会被**拷贝**（可能不止一次）
2. 算法内部操作的是仿函数的 **const 副本**
3. 有状态仿函数的状态变化发生在**副本**上，外部看不到
4. 想取回最终状态 → 用 `for_each` 的**返回值**（它是最后一次操作的仿函数副本）

### 按值传递的机制

```cpp
// STL 内部等价于：
template<typename Iter, typename Func>
Func for_each(Iter first, Iter last, Func f) {  // 按值接收
    for (; first != last; ++first)
        f(*first);  // 对 const 副本调用
    return f;       // 返回最终副本
}
```

`f` 是按值传入的拷贝。如果 `operator()` 不是 const，某些编译器会报警告，且语义上不合理——你修改的是副本的状态。

### 有状态仿函数的正确用法

```cpp
struct Accumulator {
    long long sum = 0;
    void operator()(int x) const {   // const: 但这会阻止修改 sum
        // sum += x;  // 编译错误！const 成员函数不能修改成员
    }
};

// 正确做法：用 mutable 或引用外部变量
struct Accumulator2 {
    mutable long long sum = 0;  // mutable 允许 const 函数修改
    void operator()(int x) const { sum += x; }
};

auto acc = std::for_each(v.begin(), v.end(), Accumulator2{});
std::cout << acc.sum;  // 正确：返回的副本携带最终状态
```

## 常见错误（新手踩坑）

### 错误 1：`operator()` 忘加 const

```cpp
struct Bad {
    bool operator()(int x) { return x > 0; }  // non-const!
};
// std::sort(v.begin(), v.end(), Bad{});  // 某些编译器报错或警告
```

**修复**：加 `const`。

```cpp
struct Good {
    bool operator()(int x) const { return x > 0; }  // const!
};
```

### 错误 2：有状态仿函数期望外部看到中间状态

```cpp
struct Counter {
    int count = 0;
    void operator()(int) { count++; }  // 修改的是副本
};
Counter c{0};
std::for_each(v.begin(), v.end(), c);  // c 是按值传入的！
std::cout << c.count;  // 0！外部 c 没变
```

**修复**：用返回值，或用引用捕获。

```cpp
// 方式 1：用返回值
Counter result = std::for_each(v.begin(), v.end(), Counter{});
std::cout << result.count;  // 正确

// 方式 2：lambda 引用捕获
int count = 0;
std::for_each(v.begin(), v.end(), [&count](int) { count++; });
std::cout << count;  // 正确
```

### 错误 3：仿函数太大，拷贝代价高

```cpp
struct Heavy {  // 拷贝代价大
    char data[4096];
    bool operator()(int x) const { return x > 0; }
};
std::sort(v.begin(), v.end(), Heavy{});  // 每次拷贝 4KB！
```

**修复**：轻量化仿函数，重数据用指针持有。

```cpp
struct Light {
    const char* data;  // 只拷贝指针
    bool operator()(int x) const { return x > 0; }
};
```

## 新手要点（和 C 的区别）

| 方面 | C (函数指针) | C++ (仿函数) |
|------|-------------|-------------|
| 传递方式 | 按值传指针（8 字节） | 按值传对象（大小取决于类） |
| 状态 | 无状态（全局变量变通） | 有状态（成员变量） |
| const 要求 | 无（函数本身就是 const） | `operator()` 应声明 const |
| 内联 | 难（间接调用） | 可内联（类型已知） |
| 取最终状态 | 无返回值机制 | `for_each` 返回最终副本 |

## HFT 关联

- **lambda 替代手写仿函数**：`[&](const Tick& t){ ... }` 零拷贝、可内联，HFT 热路径首选
- **有状态仿函数做回测累积**：PnL 统计用 `for_each` 返回值取结果，但注意值拷贝语义——大状态用引用捕获
- **仿函数必须轻量**：热路径排序比较器不应携带大成员，每次拷贝都影响 cache

## 代码自测

### Q1: for_each 返回值

```cpp
struct Logger {
    int calls = 0;
    void operator()(int x) const { calls++; }
};
std::vector<int> v = {1, 2, 3, 4, 5};
auto result = std::for_each(v.begin(), v.end(), Logger{});
std::cout << result.calls;
```
> 输出是什么？为什么不能直接用原始 `Logger{}` 对象查看 calls？

<details>
<summary>答案</summary>

输出 **5**。`for_each` 按值接收仿函数，内部操作的是副本，返回最终副本。`result` 是那个副本，记录了 5 次调用。

直接用 `Logger{}` 看不到 calls，因为原始对象从未被调用——被调用的是它的拷贝。

**关键**：`operator()` 必须 const，但成员用 `mutable` 才能在 const 函数中修改。
</details>

### Q2: mutable 的必要性

```cpp
struct Counter {
    int n = 0;
    void operator()(int) const { n++; }  // 编译结果？
};
```
> 这段代码能编译吗？如果不能，怎么修？

<details>
<summary>答案</summary>

**不能编译**。`const` 成员函数不能修改非 `mutable` 成员。

修复：加 `mutable`。

```cpp
struct Counter {
    mutable int n = 0;
    void operator()(int) const { n++; }
};
```

或用 lambda 引用捕获（更推荐）：
```cpp
int n = 0;
std::for_each(v.begin(), v.end(), [&n](int) { n++; });
```
</details>

### Q3: 拷贝代价

```cpp
struct Big {
    int buf[1024];
    bool operator()(int a, int b) const { return a < b; }
};
std::vector<int> v(1000);
std::sort(v.begin(), v.end(), Big{});  // 有什么问题？
```

<details>
<summary>答案</summary>

**问题**：`Big` 大小约 4KB，`sort` 内部可能多次拷贝仿函数，每次拷贝 4KB。

**修复**：仿函数应只持有必要数据。比较器不需要 `buf`：

```cpp
struct Small {
    bool operator()(int a, int b) const { return a < b; }
};  // 空类，sizeof = 1（空基类优化后可为 0）
```

或直接用 lambda：`std::sort(v.begin(), v.end(), [](int a, int b) { return a < b; });`
</details>

### Q4: lambda 按值捕获

```cpp
int threshold = 100;
auto it = std::find_if(v.begin(), v.end(),
    [threshold](int x) { return x > threshold; });
threshold = 200;  // 之后改 threshold
// it 指向的元素是否受影响？
```

<details>
<summary>答案</summary>

不受影响。lambda 按值捕获 `threshold`，捕获的是**创建 lambda 时的值**（100）。之后修改外部 `threshold` 不影响 lambda 内部的副本。

如果需要共享最新值，用引用捕获 `[&threshold]`——但要注意生命周期。

**HFT**：热路径按值捕获更安全（无数据竞争），但会增加仿函数大小。
</details>

## 参考与延伸

- 上一节：[ch05-algorithms](../ch05-algorithms/README.md)
- 下一节：[Item 39 predicate 可配对](item39-predicate-adaptable.md)
- [Effective Modern C++ Item 34：lambda vs bind](../../M1-modern-cpp/01-Effective-Modern-C++/ch06-lambda-expressions/README.md)
