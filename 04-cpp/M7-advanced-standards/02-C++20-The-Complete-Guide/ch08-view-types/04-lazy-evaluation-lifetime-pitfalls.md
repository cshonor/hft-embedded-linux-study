# 视图的惰性求值与生命周期陷阱

## 惰性：管道建起来，什么都没发生

```cpp
#include <ranges>

namespace v = std::views;

std::vector<int> v_src{1, 2, 3, 4, 5};

// 这一刻：零计算。管道只是存了"谓词 + 指向 v_src 的引用"
auto pipeline = v_src
    | v::filter([](int x) { return x % 2 == 0; })
    | v::transform([](int x) { std::puts("hit"); return x * 10; });

// 计算发生在迭代时——迭代几次算几次
auto it = pipeline.begin();   // "hit" 打印一次（找到第一个偶数）
// *it == 20，后面的 4 还没碰过
```

**推论**：view 是"查询计划"不是"查询结果"。源数据变了，下一次迭代看到的就是新值：

```cpp
auto evens = v_src | v::filter([](int x) { return x % 2 == 0; });
v_src.push_back(6);   // 管道建完之后改源
// 现在迭代 evens → 2, 4, 6 ✅ 实时视图，不是快照
```

要快照就显饰物化（`ranges::to` / `copy`），别指望 view 帮你冻住数据。

## 陷阱 1：filter 的 begin() 缓存——改了源后 begin 可能过期

```cpp
auto evens = v_src | v::filter([](int x) { return x % 2 == 0; });
auto b = evens.begin();      // filter 内部缓存了这个迭代器（惰性 begin 是 O(n) 的，
                             // 所以标准允许缓存）
v_src.front() = 999;          // 把第一个元素改成奇数
// *b —— UB/陈旧！缓存的 begin 还指着旧位置，而"第一个偶数"已经变了
```

**纪律**：filter view 的 `begin()` 结果别跨"源被修改"使用；要么先迭代完，要么改完源重新 `begin()`。

## 陷阱 2：const view 常常不能迭代

```cpp
void print(const auto& rng) {
    for (int x : rng) { /* ... */ }   // filter view 传进来可能编译失败！
}

auto evens = v_src | v::filter(pred);
print(evens);   // ❌ filter_view 的 begin() 不是 const 成员（要缓存）
```

原因：filter/drop 等 view 的 `begin()` 需要**修改内部缓存**，所以只有非 const 重载。解法：

```cpp
void print(std::ranges::view auto rng) {           // 按值传（view 拷贝 O(1)）
    for (int x : rng) { /* ... */ }
}
// 或参数写成 std::ranges::range auto&（非 const 引用）
```

这也是"view 按值传参"这条惯例的来源之一。

> **实测（clang 23 + libc++）**：`requires { cv.begin(); }` 对 const `filter_view` 探测结果为否（编译期确认无 const begin 重载）；同一管道在源 `push_back` 后重新迭代确实看到新元素（2 4 6 → 2 4 6 8），证实"实时视图非快照"。

## 陷阱 3：transform 返回临时对象 → 悬垂引用

```cpp
auto strs = v_src | v::transform([](int x) { return std::to_string(x); });

// for (const std::string& s : strs)   // 每次解引用产生一个临时 string
//     use(s);                          // 循环体内用没问题（临时活到语句结束）

// 但存起来就完了：
const std::string* p = nullptr;
for (const auto& s : strs) { p = &s; break; }
// p 悬垂——s 绑的是 transform 产生的临时，迭代器一前进就析构
```

## 陷阱 4：管道里引用短命对象

```cpp
auto bad = std::vector{1, 2, 3}
         | v::filter([](int) { return true; });
// ✅ 这个其实安全——views::all 把右值包成 owning_view（见 ch07）
// ❌ 真正危险的是引用临时函数的返回引用：
auto& front_ref = *std::vector{9}.begin();  // （与 view 无关，但同类）→ 悬垂

// view 版：谓词闭包捕获悬垂引用
auto make_pipeline(const Config& cfg) {
    return v_src | v::filter([&cfg](int x) { return x > cfg.threshold; });
    // ⚠ cfg 按引用捕获——调用方的 cfg 死了，view 拿着死引用
}
```

**纪律**：谓词捕获**按值**捕获配置，或保证被捕获对象活得比 view 的所有迭代都久。

## 陷阱 5：view 存进成员变量

```cpp
struct Bad {
    std::vector<int> data;
    std::ranges::filter_view<...> evens;  // 类型名都写不出来（实现细节类型）
    // 而且 data 一旦 reallocate，evens 里缓存的迭代器全废
};
```

view 的正确归宿是**局部变量 + 立即消费**。需要跨作用域传递时用 `std::span`（连续数据）或类型擦除的 `std::generator`（C++23，序列流），别把实现细节类型的 view 塞进结构体。

## 自查清单

| 检查项 | 规则 |
|---|---|
| 源改了之后还用旧 begin？ | ❌ 重新 begin |
| view 按 const& 传参？ | ❌ 按值传 |
| transform 的产出存指针/引用？ | ❌ 只在循环体内用 |
| 谓词按引用捕获短命对象？ | ❌ 按值捕获 |
| view 存进成员/容器？ | ❌ 用 span 或物化 |
| 需要快照语义？ | 显式 `ranges::to` |

## 自测题

1. "view 是查询计划不是结果"——用 push_back 后重新迭代的例子说明实时视图和快照的区别；要快照怎么办？
2. filter_view 的 begin() 为什么可能被缓存？由此引出哪两条使用纪律（改源、const 传参）？
3. 为什么 `print(const auto& rng)` 对 filter view 编译失败？两种修法？
4. transform 产生临时对象时，循环体内使用安全而存指针悬垂，边界在哪（临时对象的生命周期规则）？
5. 为什么 view 不适合存进成员变量？需要跨作用域时的两个替代方案？
