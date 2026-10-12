# 即席约束与 requires 子句进阶

## 不定义 concept 也能约束：即席 requires

不是每个约束都值得起名字。一次性约束直接写在 `requires` 子句里：

```cpp
#include <concepts>

// 1. 直接用类型特征（不用包装成 concept）
template <typename T>
    requires std::is_trivially_copyable_v<T>
void send_raw(const T& msg, int fd);

// 2. 即席 requires 表达式：约束体当场写
template <typename T>
    requires requires(T t) {
        t.on_tick();                    // 要求有 on_tick()
        { t.id() } -> std::convertible_to<int>;
    }
void register_strategy(T& strat);

// 3. 组合：&& / || / !
template <typename T>
    requires (std::integral<T> && !std::same_as<T, bool>)
void push_field(T value);   // 整数但不要 bool（bool 进位段太容易写错）
```

**纪律**：同一个约束写第二遍时，就提命名 concept——`requires requires` 两层嵌套可读性掉得很快。

## requires 子句可以放哪（比想象中多）

```cpp
// 1. 函数模板声明后（最常见）
template <typename T> requires std::integral<T>
T saturating_add(T a, T b);

// 2. 模板参数列表前（少见但合法）
template <typename T>
    requires std::integral<T>
struct RingBuffer;

// 3. 非模板函数也能加！——约束它所属类的模板参数
template <typename T>
struct OrderBook {
    // 只有 T 是双精度时才提供 vwap()——成员按需存在
    double vwap() const
        requires std::floating_point<T>;
};

// 4. lambda 的模板参数
auto cmp = []<std::totally_ordered T>(const T& a, const T& b) {
    return a < b;
};

// 5. 约束模板的模板参数（template template parameter）
template <template <typename> typename C>
    requires requires(C<int> c) { c.push_back(1); }
struct Wraps;
```

第 3 点是写库的关键手法：**成员函数按约束"按需实例化"**——`OrderBook<Price>` 有 `vwap()`，`OrderBook<RawMsg>` 没有，而且不叫 SFINAE、不影响类其余成员。

## 否定约束 ! 与"这个重载不许选"

```cpp
// 常见坑：转发引用吃掉拷贝构造
template <typename T>
struct Wrapper {
    Wrapper(const Wrapper&) = default;

    // 完美转发构造——不加约束会把 Wrapper& 也匹配走
    template <typename U>
        requires (!std::same_as<std::remove_cvref_t<U>, Wrapper>)
    Wrapper(U&& u) : val(std::forward<U>(u)) {}

    T val;
};
```

`std::remove_cvref_t` 是 C++20 专门为此加的——剥掉 `&`/`const`/`volatile` 再比类型。**写转发构造函数，这个否定约束是标配**，否则 `Wrapper w2(w1)` 会走转发模板（U 推导成 Wrapper&）而不是拷贝构造。

## requires 表达式里的四种要求

```cpp
template <typename T, typename U>
concept FeedParser = requires(const T& parser, const U& raw) {
    // 1. 简单要求：表达式合法
    parser.parse(raw);

    // 2. 类型要求：这个嵌套类型存在
    typename T::result_type;

    // 3. 复合要求：合法 + 结果类型约束 + noexcept
    { parser.parse(raw) } -> std::convertible_to<typename T::result_type>;
    { parser.reset() } noexcept;

    // 4. 嵌套要求：里面再套一层 requires 子句
    requires std::default_initializable<typename T::result_type>;
};
```

`{ ... } noexcept` 在 HFT 里有实际用途：**约束 swap/比较必须 noexcept**，否则编译期拒绝——异常规格直接编进接口。

## 实战：约束热的还是约束冷的？

编译期约束没有运行开销，但有**编译时间**和**接口稳定性**成本：

```cpp
// ✅ 值得约束：接口边界——错误信息在调用点就清晰
template <std::integral T>
struct alignas(64) SeqCounter { /* ... */ };

// ❌ 过度约束：内部实现细节全写成 concept，改一处连锁改十处
//    内部模板用 static_assert 反而更直接：
template <typename T>
struct internal_pool {
    static_assert(std::is_trivially_destructible_v<T>,
                  "pool 只放平凡类型");   // 约束失败一样是编译错误
};
```

经验法则：**头文件暴露给别人的模板用 concepts，.cpp 内部用 static_assert**。前者是文档+重载分派工具，后者是断言。

## 自测题

1. `requires requires` 两层分别是什么？为什么一次性约束可以这么写、复用约束不该这么写？
2. 非模板成员函数加 requires 子句约束的是什么？`OrderBook<T>::vwap` 例子的实例化行为？
3. 转发构造函数为什么必须配 `!std::same_as<std::remove_cvref_t<U>, Wrapper>` 否定约束？不配会发生什么？
4. 复合要求 `{ x } noexcept` 把什么编进了接口？对 swap 场景的意义？
5. "头文件用 concepts、内部用 static_assert" 的理由是什么？两者编译错误信息出现的位置有何不同？
