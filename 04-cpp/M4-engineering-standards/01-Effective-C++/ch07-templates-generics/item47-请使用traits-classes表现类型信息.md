# 条款 47：请使用 traits classes 表现类型信息

## 本节讲什么

**Use traits classes for information about types.** traits 是"类型的元数据表"——
把"关于类型 T 的事实"（迭代器类别/是否指针/加减语义）集中到
一个模板结构里，让泛型代码**按类型特性分派行为**。
标准库的 `iterator_traits` 是开山之作，`type_traits`（→ 19.8）是它的现代化。
本条讲清 traits 的结构、tag dispatch 手法、以及与 C++17 后工具的代际关系。

← 上一条 [item46 模板非成员函数转换](./item46-需要类型转换时请为模板定义非成员函数.md)；
下一章 [ch08 new 与 delete](../ch08-new-delete/)。

---

## 1. traits 的标准结构（iterator_traits 示范）

```cpp
// 主模板：默认从迭代器自身取（迭代器类内部自带五件套 typedef）
template <typename Iter>
struct iterator_traits {
    using iterator_category = typename Iter::iterator_category;
    using value_type        = typename Iter::value_type;
    using difference_type   = typename Iter::difference_type;
    // ...
};

// 指针特化：裸指针没有内部 typedef，traits 补给它
template <typename T>
struct iterator_traits<T*> {
    using iterator_category = std::random_access_iterator_tag;
    using value_type        = T;
    using difference_type   = std::ptrdiff_t;
};
```

**设计精髓**：泛型代码只问 `iterator_traits<It>::xxx`——
不管 It 是迭代器类还是裸指针，**答案总有一个统一入口**。
"类型的信息"被从"类型的实现"里抽出来，独立成一层。

## 2. tag dispatch：traits 的经典用法（按类别分派）

```cpp
// advance 对随机访问迭代器是 O(1)，对双向是 O(n)——按 traits 选实现
template <typename Iter>
void advance_impl(Iter& it, std::ptrdiff_t n, std::random_access_iterator_tag) {
    it += n;                                    // O(1)
}
template <typename Iter>
void advance_impl(Iter& it, std::ptrdiff_t n, std::bidirectional_iterator_tag) {
    while (n-- > 0) ++it;                       // O(n)
}
template <typename Iter>
void advance(Iter& it, std::ptrdiff_t n) {
    advance_impl(it, n,
        typename iterator_traits<Iter>::iterator_category{});   // tag 当实参，重载决议分派
}
```

tag 是**空结构体**（`struct random_access_iterator_tag {};`）——
它的唯一使命是当重载决议的"类型标签"，零运行期成本。
（C++17 起 `if constexpr` + traits 常能替代 tag dispatch，→ 19.8 ①。）

## 3. 给自己的类型写 traits（工程用法）

```cpp
// 协议消息类型 → 它的元数据
template <typename T> struct msg_traits;                  // 主模板：无定义（未支持的类型直接报错）

template <> struct msg_traits<SnapMsg> {
    static constexpr uint16_t type_id = 0x01;
    static constexpr bool has_seq = true;
    using header_type = SnapHeader;
};
template <> struct msg_traits<TradeMsg> {
    static constexpr uint16_t type_id = 0x02;
    static constexpr bool has_seq = false;
};

// 泛型解码骨架按 traits 工作
template <typename T>
void dispatch(const Buf& b) {
    static_assert(msg_traits<T>::type_id != 0, "未注册的消息类型");
    if constexpr (msg_traits<T>::has_seq) { check_seq(b); }
    decode<typename msg_traits<T>::header_type>(b);
}
```

**traits vs 类内常量**（`T::type_id`）：traits 是**外挂**的——
不给类型改代码就能补元数据（第三方类型/旧协议类型没法改），
且同类型可在不同上下文挂不同 traits。

## 4. 代际关系：traits → type_traits → concepts

| 代际 | 工具 | 形态 |
|---|---|---|
| C++98/03（本条语境） | 手写 traits + tag dispatch | 元数据表 + 标签重载 |
| C++11/14 | `<type_traits>`（is_integral/conditional…） | 标准化的 traits 集（→ 19.8） |
| C++17 | `if constexpr` + `_v` 变量模板 | 分派逻辑写人话 |
| C++20 | **concepts** | 元数据升级为约束（→ item41） |

理解 traits 是理解后三代的地基——concepts 的 `requires` 表达式
本质上就是"按需生成的 traits"。

## HFT 关联

- **协议消息注册表**用 traits 而不是类内常量：交易所协议类型来自
  生成的代码/第三方头，改不动——外挂 traits 是唯一选择（③ 的 dispatch 骨架）
- `if constexpr (msg_traits<T>::has_seq)` 是"协议族解码器"的标准写法：
  编译期按消息特性裁剪代码，零运行期分支（→ 19.8 ①）
- 自定义迭代器（订单簿档位迭代器）必须自带五件套 typedef——
  否则 STL 算法（`std::lower_bound`）经 iterator_traits 拿不到信息，
  泛型算法全部拒用

## 代码自测

**题目 1：** `iterator_traits<T*>` 特化存在的根本原因是什么？

<details>
<summary>参考答案</summary>

裸指针没有内部 typedef——`int*` 里写不了 `int*::value_type`。
但泛型算法（`std::advance`/`std::distance`）必须同时服务迭代器类和裸指针。
traits 的指针特化把"指针这种类型"的元数据**外接**进来：
`iterator_traits<int*>::iterator_category = random_access_iterator_tag`——
于是 `int*` 在泛型世界里获得了与迭代器类平起平坐的"元数据身份"。
这就是 traits 的本质：**类型的信息不一定要住在类型里，外挂一层表，统一入口**。

</details>

**题目 2：** tag dispatch 和 `if constexpr` 解决同一个问题，各自的时代与代价？

<details>
<summary>参考答案</summary>

tag dispatch（C++98 起）：用**空结构体标签**做重载决议的实参，
把"按特性选实现"翻译成函数重载——零运行期成本，但要写 N+1 个函数
（N 个 impl + 1 个入口），逻辑被拆散，新特性要加函数。
`if constexpr`（C++17 起）：分派写在**一个函数**里，
被丢弃分支不实例化——逻辑集中可读，但不能像重载那样开放扩展
（新特性要改函数体）。
现代代码：库内部分派优先 if constexpr；需要"用户可为新类型扩展行为"的
开放分派点（如自定义 traits 特化）仍用 tag/traits 特化。

</details>

**题目 3：** 为什么协议消息的元数据（type_id/has_seq）该放 traits 而不是消息类内部？

<details>
<summary>参考答案</summary>

① **类型改不动**：协议消息类常来自代码生成器（protobuf/自研 IDL）或第三方头——
加类内常量要改生成的代码，下次重新生成就被覆盖；
② **元数据随上下文变**：同一消息类型在"解码上下文"和"风控上下文"可能需要
不同的元数据视图——类内常量只能有一份，traits 可以按上下文各挂一套
（`decode_traits<T>` vs `risk_traits<T>`）；
③ **主模板无定义的编译期拦截**：未注册类型直接编译错误
（"msg_traits<X> 不完整"）——比类内常量的"默认值被静默继承"安全。
traits 把"类型的元数据"从"类型的实现"里解放出来——
这正是本条"用 traits 表现类型信息"的完整含义。

</details>
