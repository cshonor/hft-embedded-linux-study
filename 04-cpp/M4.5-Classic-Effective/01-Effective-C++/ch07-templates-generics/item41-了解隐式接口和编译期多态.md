# 条款 41：了解隐式接口和编译期多态

## 本节讲什么

**Understand implicit interfaces and compile-time polymorphism.** OOP 的接口是
**显式**的（基类虚函数签名），多态在**运行期**；模板的接口是**隐式**的
（"这个类型只要有这些操作就行"——鸭子类型），多态在**编译期**。
两套多态机制的思维切换，是读懂模板错误信息和写泛型代码的门槛。

← 上一条 [item40 慎用多重继承](../ch06-inheritance-oop/item40-明智而审慎地使用多重继承.md)；
下一条 [item42 typename 的双重意义](./item42-了解typename的双重意义.md)。

---

## 1. 两套接口、两套多态（对照总表）

| | OOP（显式接口 + 运行期多态） | 模板（隐式接口 + 编译期多态） |
|---|---|---|
| 接口声明 | 基类虚函数签名（写在类型里） | **不出现**——模板代码用到什么，接口就是什么 |
| 约束检查时机 | 编译期（派生类签名必须匹配） | **实例化时**（用了没有的操作才报错） |
| 类型兼容 | is-a 继承关系 | **鸭子类型**：长得像就行，无需共同基类 |
| 错误信息 | 短而准（签名不匹配） | 长而绕（实例化深处，→ item47 改善） |
| 开销 | vtable 间接 | **零间接**（直接调用，可内联） |
| 异构集合 | 天然（基类指针容器） | 不能（每个实例化是独立类型，→ variant 补救） |

## 2. 隐式接口的庐山真面目

```cpp
template <typename T>
void process(T& w) {
    if (w.size() > 1 && w != last) {     // T 的隐式接口就此形成：
        T tmp(w);                        //   size()、operator!=、拷贝构造
        w.normalize();                   //   normalize()
        w.swap(tmp);                     //   swap(T&)
    }
}
```

**"T 必须支持什么"没有任何一处显式声明**——它由函数体里**实际用到的表达式**决定：
哪个类型让这些表达式全部合法，哪个类型就能实例化。
`int` 不行（没有 size()），`std::string`/`vector` 可以——
**接口是代码"用"出来的，不是"声明"出来的**。

推论：同一个模板对不同 T，"接口要求"可能不同——
`if` 分支没走到的操作对那个 T 就不是必需的（C++17 `if constexpr` 把这个变成了
编译期可裁剪的显式能力，→ 19.8 ①）。

## 3. 编译期多态的形状（对比虚函数）

```cpp
// OOP：一个函数，运行期决定行为
void draw(const Shape& s) { s.draw(); }        // vtable 跳转

// 模板：每个 T 实例化一份，编译期决定行为
template <typename T>
void draw_t(const T& s) { s.draw(); }          // 直接调用——可内联、零间接
```

编译期多态的代价转移：
- **收益**：零 vtable、内联机会、类型特化优化（`vector<int>` 与 `vector<bool>` 完全不同实现）
- **代价**：每个 T 一份代码（膨胀）、T 之间无共同基类（异构容器要 variant/type erasure）、
  接口错误在实例化深处爆炸

## 4. C++20 concepts：给隐式接口一个显式写法（前瞻）

```cpp
template <typename T>
concept Normalizable = requires(T& t, const T& c) {
    { t.size() } -> std::convertible_to<std::size_t>;
    { t.normalize() };
    { t.swap(t) };
};
template <Normalizable T>
void process(T& w) { /*...*/ }      // 接口显式化：错误信息从"实例化深渊"变"概念不满足"
```

C++17 及以前的折中：`static_assert` + traits 守卫（→ 19.8 ④ 的 send 例）。
C++20 起隐式接口可以显式声明——但"鸭子类型 + 编译期多态"的**本质不变**，
concepts 只是给鸭子写了物种鉴定书（→ M5 C++20 ch07）。

## HFT 关联

- 热路径泛型（`RingBuffer<T>`/`OrderBook<Depth>`）天然是编译期多态：
  零间接 + 按类型特化布局——虚函数那层间接在 tick-to-trade 里是纯粹的浪费
- 模板策略基类（CRTP，item35）= 隐式接口的工程化：基类约定
  "Derived 必须有 do_on_tick()"，不写虚函数不写签名——
  文档 + static_assert 把隐式接口钉住
- 异构集合需求出现时，别把编译期多态硬拗成继承——
  `std::variant<A, B, C>` + `std::visit` 是"类型集合封闭的编译期多态"的标准容器形态
- 泛型代码的接口文档责任：隐式接口**不会自己说话**——
  模板参数的类型要求必须写在注释/concepts 里（"T 必须 trivially copyable"等，
  → 19.8 守卫模式）

## 代码自测

**题目 1：** 模板的"隐式接口"和基类的"显式接口"在错误报告上有什么本质区别？

<details>
<summary>参考答案</summary>

显式接口（基类虚函数）：派生类签名不匹配在**编译派生类时**报错，
错误指向"你的签名和基类不一致"——短、准、指向接口本身。
隐式接口（模板）：类型缺少操作在**实例化时**报错——
错误从模板函数体深处开始展开（"process() 中 w.normalize() 无此成员"，
附上一整串实例化栈）——长、绕、指向使用点而非接口定义。
C++20 concepts 把隐式接口显式化后，错误收敛为"类型不满足概念 Normalizable"——
这就是给隐式接口补的显式声明层。

</details>

**题目 2：** 编译期多态（模板）相比运行期多态（虚函数），两个核心优势和一个核心代价是什么？

<details>
<summary>参考答案</summary>

优势① **零间接**：直接调用可内联，无 vtable 跳转（热路径决定性收益）；
优势② **类型特化优化**：每个 T 独立实例化，可以为特定类型生成完全不同的
实现（`vector<bool>` 的位压缩）——运行期多态做不到"按类型换实现"。
代价：**异构集合能力丧失**——`MyTemplate<A>` 和 `MyTemplate<B>` 是无关类型，
放不进同一个基类指针容器；要异构集合得退回 variant（封闭集）或
type erasure（开放集，又引入一层间接）。
"编译期多态换运行期灵活性"是这笔交易的本质。

</details>

**题目 3：** 写泛型函数 `log_value(const T& v)`，要求 T 可流输出——
C++17 和 C++20 各怎么把"接口要求"表达清楚？

<details>
<summary>参考答案</summary>

C++17（static_assert + traits/void_t，→ 19.8 ②）：

```cpp
template <typename T>
void log_value(const T& v) {
    static_assert(has_ostream_insertion_v<T>,
                  "log_value 要求 T 支持 operator<<（可流输出）");
    std::cout << v;
}
```

C++20（concept）：

```cpp
template <typename T>
concept Streamable = requires(std::ostream& os, const T& v) { os << v; };
void log_value(const Streamable auto& v) { std::cout << v; }
```

共同点：隐式接口本身不变（还是鸭子类型），变的是**把要求写成显式契约**——
错误信息从实例化深处提前到调用点，模板的使用门槛从"读实现"降为"读契约"。

</details>
