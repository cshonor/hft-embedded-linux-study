# 条款 42：了解 typename 的双重意义

## 本节讲什么

**Know the two meanings of typename.** `typename` 有两个毫不相干的职责：
① 声明模板类型参数（与 `class` 等价）；② 告诉编译器"**依赖名是类型**"
（`T::xxx` 可能是类型也可能是静态成员，编译器需要你帮它消歧）。
第二条不写就是编译错误——这是模板初学者的第一道坎。本机 g++ 13.3 实测。

← 上一条 [item41 隐式接口](./item41-了解隐式接口和编译期多态.md)；
下一条 [item43 模板基类的名称查找](./item43-学习处理模板化基类内的名称.md)。

---

## 1. 第一义：声明类型参数（与 class 等价）

```cpp
template <typename T> void f();   // ✅
template <class T> void f();      // ✅ 完全等价——历史遗留，class 不代表"只能是类类型"
```

- `class T` 里的 class 是**历史包袱**（模板诞生于 typename 关键字之前）——
  `int`/`double` 照样能实例化 `template <class T>`
- 社区惯例：现代代码统一写 `typename`（语义诚实）；模板模板参数在 C++17 前
  **只能**写 `class`（本机实测 C++17 起 typename 也行，见 ③）

## 2. 第二义：依赖名消歧（不写就报错）

```cpp
template <typename T>
void demo(const T& container) {
    typename T::const_iterator it = container.begin();   // typename 必须！
}
```

**为什么必须**：`T::const_iterator` 是**依赖名**（dependent name——
它依赖模板参数 T，实例化前不知道是什么）。编译器解析模板时默认假定
依赖名是**值**（静态成员/枚举值），不是类型：

```cpp
T::const_iterator * x;   // 没有 typename：编译器理解为 "(T::const_iterator) * x"——
                         // 两个"值"相乘！加 typename 才理解为"指针声明"
```

**规则**：`::` 左边是**依赖模板参数**的名字，且整体要当**类型**用时，
必须冠 `typename`——`typename T::const_iterator`、`typename T::value_type`、
`typename std::vector<T>::iterator`。

**三个例外（不用写）**：
1. 基类列表里：`class D : public T::Base { }`（不允许写）
2. 成员初始化列表：`D() : T::Base() { }`（不允许写）
3. 名字不依赖模板参数时：`std::vector<int>::iterator`（T 已定，不是依赖名）

**本机实测**：`typename T::const_iterator it = container.begin();` 编译运行通过，
输出 `first=42`。

## 3. 模板模板参数的 class/typename 史（本机实测）

```cpp
// C++17 前：模板模板参数只能用 class
template <template <typename, typename> class Container, typename V>
void demo2(const Container<V, std::allocator<V>>& c);
// C++17 起 typename 也可以（本机实测 g++ 13.3 通过：demo2(v) 输出 size=2）
```

知道这个史就能看懂老代码里"为什么这里偏偏写 class"。

## 4. 与 item43 的联动（template 关键字兄弟）

依赖名是**模板**时，还有另一个消歧关键字：`x.template f<T>()`——
`typename` 管"依赖名是类型"，`template` 管"依赖名是模板"（→ item43 详述）。

## HFT 关联

- 泛型容器（`RingBuffer<T>`/`OrderBook<Depth>`）内部到处是依赖名：
  `typename Alloc::template rebind<T>::other` 这类经典长链是 allocator
  编程的日常——typename/template 两个消歧写错一个，编译错误直接淹没你
- 读标准库实现（`std::vector` 的 `_Alloc_traits::pointer` 等）必须条件反射
  "依赖名要 typename"——这是阅读模板库代码的识字能力
- C++20 起依赖名规则放宽（很多语境不再需要 typename），
  但 C++17 代码库（本仓主力）里它仍是必修课

## 代码自测

**题目 1：** `template <class T>` 和 `template <typename T>` 有区别吗？
`class T` 意味着 T 必须是类类型吗？

<details>
<summary>参考答案</summary>

**完全等价**，且 `class T` **不**限制 T 必须是类类型——`int`、`double`、指针
都能实例化。`class` 在这里是历史遗留：模板诞生时还没有 typename 关键字，
借用了 class；后来加入 typename 才是语义正确的写法（"一个类型参数"）。
现代代码惯例统一 typename；看到老代码的 class 知道它不是"类限定"即可。
（C++17 前唯一必须用 class 的角落：模板模板参数，本机实测 C++17 已解禁。）

</details>

**题目 2：** 为什么 `T::const_iterator` 需要 typename 而 `std::vector<int>::const_iterator` 不需要？

<details>
<summary>参考答案</summary>

前者是**依赖名**：`T` 是未知模板参数，编译器解析模板时不知道
`T::const_iterator` 是类型还是静态成员值——默认按值处理，
`T::const_iterator * x` 会被解析成乘法表达式。typename 是你替编译器做的消歧。
后者的 `std::vector<int>` 是**完全确定的类型**（不依赖任何模板参数）——
编译器当场就能查到 `const_iterator` 是类型定义，无需消歧。
规则一句话：`::` 左边依赖模板参数 + 整体当类型用 → 冠 typename。

</details>

**题目 3：** 下面这段有哪两处错误？

```cpp
template <typename T>
class D : public typename T::Base {
    typename T::value_type m_;
public:
    D() : typename T::Base() {}
    typename T::value_type get() { return m_; }
};
```

<details>
<summary>参考答案</summary>

两处**多余的 typename**（规则的两个例外）：
① 基类列表里**不能**写 typename：`class D : public T::Base`（正确写法）；
② 成员初始化列表里**不能**写 typename：`D() : T::Base() {}`。
成员声明 `typename T::value_type m_;` 和返回类型里的 typename 是**必须的**
（依赖名当类型用，且不在例外之列）。
记法：typename 只出现在"声明/使用类型"的普通位置；
继承列表和构造初始化列表是禁写区。

</details>
