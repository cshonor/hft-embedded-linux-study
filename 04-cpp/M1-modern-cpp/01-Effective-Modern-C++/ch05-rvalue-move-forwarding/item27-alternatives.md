# Item 27：万能引用重载的替代方案

> 第 5 章 · Item 27 · 上一节：[Item 26 避免万能引用重载](item26-avoid-overloading-universal.md)

## 为什么要学这个（先建立直觉）

Item 26 告诉你"别给万能引用开重载"，因为它太贪婪——精确匹配的普通重载都可能抢不过它。但需求不会消失：你还是要写"什么类型都能进、不同类型走不同逻辑"的接口。这一节给三种替代方案，从"绕开重载决议"到"给模板加约束"到最务实的"干脆别用模板"。

先看问题现场。经典的 Meyers 例子（clang 23.1.0 实测）：

```cpp
class Person {
public:
    template<typename T>
    explicit Person(T&& n) : name(std::forward<T>(n)) {}
    Person(const Person& rhs) : name(rhs.name) {}
    std::string name;
};

Person p2("x");
Person p3(p2);   // p2 是非 const 左值 → 拷贝构造（const Person&）要加 const，
                 // 万能引用构造 T=Person& 是零转换精确匹配 → 劫持！
                 // 然后 string 拿 Person 构造 → 报一个莫名其妙的错：
// error: no matching constructor for initialization of 'std::string'
```

调用方写的是"拷贝一个 Person"，报错却在 `std::string` 的构造上——错误位置离真凶隔着一层模板。这就是 Item 26 说的问题，也是本节三种方案要消灭的东西。

---

## 这节讲什么

1. 标签分发（tag dispatch）：把分发从重载决议手里抢回来；
2. `enable_if` 约束（C++11/14 的正解）与 Concepts（C++20 的现代正解）——实测劫持被治好；
3. 方案三：放弃万能引用，用具名重载——以及什么时候这就够了。

---

## 方案一：标签分发（tag dispatch）

思路：万能引用入口保留（来者不拒），但**不在入口层面做重载**——把分发推迟到内部，用一个编译期"标签"参数走普通重载决议：

```cpp
#include <type_traits>

void log_impl(const std::string& msg, std::true_type)  { /* 整型专用通道 */ }
void log_impl(const std::string& msg, std::false_type) { /* 通用通道 */ }

template<class T>
void log(T&& msg) {
    if constexpr (std::is_integral_v<std::remove_reference_t<T>>)
        log_impl(std::to_string(msg), std::true_type{});   // 标签类型 true_type
    else
        log_impl(std::forward<T>(msg), std::false_type{}); // 标签类型 false_type
}
```

`std::true_type` 和 `std::false_type` 是两个不同的空类——`log_impl` 的两个重载靠**第二个参数的类型**区分，走的是最普通、最可预测的重载决议，跟"万能引用抢不抢"完全无关。实测：`log(42)` 走整型通道，`log(std::string("tick"))` 走通用通道。

要点：**贪婪只发生在入口一次**，内部用标签把"该走哪条路"变成显式的编译期判断。C++17 的 `if constexpr` 让分支不写进实例化，更干净；老代码里你会看到不带 `if constexpr` 的纯标签分发（把条件塞进标签类型本身），机制相同。

## 方案二：enable_if / Concepts 约束模板

思路：不给万能引用开重载，而是**给万能引用上镣铐**——它在某些类型面前直接"不存在"：

```cpp
#include <type_traits>

class PersonFixed {
public:
    template<typename T,
             typename = std::enable_if_t<
                 !std::is_same_v<std::decay_t<T>, PersonFixed>>>
    explicit PersonFixed(T&& n) : name(std::forward<T>(n)) {}

    PersonFixed(const PersonFixed& rhs) : name(rhs.name) {}
    std::string name;
};
```

`enable_if_t<条件>` 在条件为假时让整个模板**从候选集里消失**（SFINAE）：`T` 推导为 `PersonFixed&` 时约束不满足，这个构造函数模板就像没写过——拷贝构造正常接手。实测：`PersonFixed f2(f1);`（非 const 左值）走的是拷贝构造，劫持消失。

注意 `std::decay_t<T>`：必须先把 `T&`、`const T&` 剥成 `PersonFixed` 再比较，否则 `T=PersonFixed&` 时 `is_same` 判定失败、约束形同虚设。这是 enable_if 写法最容易漏的细节。

C++20 用 Concepts 把镣铐写成人话（clang 23.1.0 + `-std=c++20` 实测通过）：

```cpp
class PersonC {
public:
    template<typename T>
        requires (!std::is_same_v<std::decay_t<T>, PersonC>)
    explicit PersonC(T&& n) : name(std::forward<T>(n)) {}

    PersonC(const PersonC&) = default;
    std::string name;
};
```

`requires` 子句语义和 enable_if 相同，但：约束写在明处、可读、可命名复用（`template<std::integral T>`），报错从 SFINAE 实例化噪声变成"约束未满足"一句话。新项目直接用 Concepts，enable_if 只留给维护 C++17 及以前的代码。

## 方案三：放弃万能引用重载（务实首选）

```cpp
void set(const std::string& s);
void set(int idx);
void set(OrderId id);
```

需求如果就是"接这几种已知类型"，直接用具名重载：无模板、无推导、无 SFINAE，行为一眼到底，报错一句话说清。代价是多写几个重载、字符串字面量可能多一次构造——但换来的是**零意外**。

---

## 常见错误（新手踩坑）

1. **enable_if 忘写 `decay_t`**。`is_same_v<T, Person>` 在 `T = Person&` 时恒假——左值劫持依旧。比较前一律 `std::decay_t<T>`。
2. **以为标签分发过时了**。`if constexpr` 能覆盖的场景变多，但标签分发在"分发条件复杂、要静态断言、要留扩展点"时仍然是最显式的方案——标准库的迭代器分发（`std::advance` 按迭代器类别）就是标签分发的活化石。
3. **给构造函数用 enable_if 时放错位置**。构造函数没有返回类型，`enable_if` 只能放模板默认实参（`typename = std::enable_if_t<...>`），不能放返回类型处——这是构造场景特有的写法。
4. **能用具名重载解决的非要上模板**。三个类型的分发写成万能引用 + 约束，是把简单问题复杂化。方案三不是"新手村"，是默认选项；前两方案是确认重载真的不够用时才上。

---

## 新手要点

- **学习顺序**：先用方案三（具名重载）建立"多数需求不需要模板"的直觉 → 再学 Concepts（现代、直白）→ enable_if 读老代码够用即可 → 标签分发理解机制即可。
- **SFINAE 一句话**："替换失败不是错误"——模板实参替换出非法类型时，该模板安静出局，不报错。enable_if 就是故意制造替换失败的工具。
- **C++20 分水岭**： Concepts 不是 enable_if 的语法糖，是把"约束"提升为语言一级概念；能写 C++20 就别写 enable_if。

---

## HFT 关联

- **行情解码分发**：`template<class T> void onMessage(T&& msg)` 按消息类型分发解码器——用标签分发或 Concepts 约束，避免万能引用吞掉"本该精确匹配的专门重载"。行情 handler 的错配是静默的（能编译、跑错逻辑），比编译错更贵。
- **接口约束即文档**：`requires std::is_same_v<std::decay_t<T>, Tick>` 写在签名上，调用方一看就知道接口接受什么——热路径代码的接口显式性比少写两行重要。

---

## 自测题

1. `Person p3(p2)`（非 const 左值）为什么会被万能引用构造劫持？实测报什么错？
2. 标签分发如何绕开"万能引用贪婪匹配"？标签本身是什么？
3. `enable_if` 约束里为什么必须写 `std::decay_t<T>` 而不是直接比 `T`？
4. C++20 用什么替代 enable_if？写出 Person 约束构造的 Concepts 版本。
5. 什么时候方案三（具名重载）优于前两种模板方案？

<details>
<summary>参考答案</summary>

1. 拷贝构造形参是 `const Person&`，非 const 左值 `p2` 绑它要追加 const（一次限定转换）；万能引用构造 `T=Person&` 是零转换精确匹配，重载决议胜出——于是模板体里 `std::string` 拿 `Person` 构造，报 `error: no matching constructor for initialization of 'std::string'`（实测原文）。错误落在 `string` 构造上，离真凶（重载被抢）隔一层模板，极难排查。

2. 万能引用只做入口（来者不拒但不做决策），内部把实参 `forward` 给一个带**编译期标签参数**的 impl 重载集：`log_impl(x, std::is_integral<T>{})`。标签是空类（`std::true_type`/`std::false_type` 或自定义 trait 类型），不同类型触发普通重载决议——分发由标签类型决定，与万能引用的匹配优先级完全无关。

3. 万能引用接左值时 `T` 推成 `Person&`（接 const 左值推成 `const Person&`）——直接 `is_same_v<T, Person>` 恒为假，约束永远挡不住想挡的类型。`std::decay_t` 把引用、const、数组/函数形态全剥回 `Person`，比较才有意义。这是 enable_if 约束写法最高频的疏漏。

4. Concepts / `requires` 子句：

   ```cpp
   template<typename T>
       requires (!std::is_same_v<std::decay_t<T>, Person>)
   explicit Person(T&& n) : name(std::forward<T>(n)) {}
   ```

   语义与 enable_if 相同（不满足就从候选集消失），但约束写在明处、可命名复用、报错是"约束未满足"而非一串实例化噪声。clang 23.1.0 + `-std=c++20` 实测通过，非 const 左值拷贝正确走拷贝构造。

5. 当需求是"接这几种已知类型"且类型集合稳定时：具名重载无推导、无 SFINAE、行为直读、报错直白，零意外面。模板方案（标签分发/约束）只在"类型集合开放"（泛型库接口）或"按类型性质分发"（整型/非整型）时值得。先排除简单方案再上模板，是 Meyers 的本意。

</details>

---

## 参考与延伸

- 下一节：[Item 28 引用折叠](item28-reference-collapsing.md)
- 回到：[第 5 章](README.md)
