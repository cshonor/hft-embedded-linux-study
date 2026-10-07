# 条款 25：考虑提供不抛异常的 swap 重载

## 本节讲什么

**Consider support for a non-throwing swap.** `swap` 在 C++ 里远不止"交换两个变量"：
它是**强异常保证的基石**（copy-and-swap）、**移动语义的组成件**、
**标准库对 noexcept 的探测点**。本条款讲清：什么时候该自定义 swap、
三层提供方式（成员/非成员/std::swap 特化）、以及 `using std::swap; swap(a,b);` 调用惯用法。
本机 g++ 13.3 实测惯用法与 noexcept 探测。

← 上一条 [item24 非成员函数支持全参数转换](./item24-需要所有参数都支持隐式类型转换时，使用非成员函数.md)；
下一章 [ch05 实现](../ch05-implementations/)。

---

## 1. 为什么 swap 特殊（不只是交换）

**默认 `std::swap` 的实现**（C++11 起）：

```cpp
template <typename T>
void swap(T& a, T& b) {
    T tmp = std::move(a);   // 移动构造
    a = std::move(b);       // 移动赋值
    b = std::move(tmp);     // 移动赋值
}
```

对"内部只是指针"的类型（pImpl、句柄、资源包装），三次移动是**三次指针交换**——完美。
但对深度存储的类型，移动仍可能分配内存（`std::string` 大字符串、`vector`）——
**分配就可能抛**，而 swap 的使用场景恰恰都要求"绝不抛"：

- **copy-and-swap**：`T& operator=(T tmp) { swap(tmp); return *this; }`——
  强保证的前提是 swap 不抛（拷贝在参数构造时完成，swap 只是指针过户）
- **移动构造/移动赋值**：标 `noexcept` 的实现内部几乎都用 swap 零件
  （不 noexcept 的移动 → vector 扩容退化拷贝，见 18.1 ② 实测）

## 2. 三层提供方式（本机实测）

```cpp
class Widget {
    int* data_;
public:
    // ① 成员 swap：做实际工作——交换成员指针，noexcept
    void swap(Widget& o) noexcept {
        std::swap(data_, o.data_);
    }
    Widget(Widget&& w) noexcept : data_(w.data_) { w.data_ = nullptr; }
    Widget& operator=(Widget&& w) noexcept { std::swap(data_, w.data_); return *this; }
};

// ② 同命名空间的非成员 swap：通用调用入口（ADL 命中）
void swap(Widget& a, Widget& b) noexcept { a.swap(b); }

int main() {
    Widget a, b;
    using std::swap;      // ③ 调用惯用法：让 std::swap 兜底
    swap(a, b);           // 本机实测：命中②（ADL 优先），打印 member swap
    static_assert(noexcept(swap(a, b)), "");   // 本机实测：通过 ✓
}
```

**本机实测**：`swap(a, b)` 命中自定义非成员版（ADL），`static_assert(noexcept(...))` 通过。

**调用惯用法的原理**：`using std::swap; swap(a, b);`——
ADL 先找 `Widget` 所在命名空间的 swap（自定义优先），找不到才用 `std::swap` 兜底。
**直接写 `std::swap(a, b)` 会跳过 ADL**，自定义版本被无视——这是泛型代码里的大忌
（→ 18.2 ⑤ ADL 的工程推论）。

## 3. 什么时候必须自定义 swap（判别）

| 你的类型 | 需要自定义 swap 吗 |
|---|---|
| 成员全是"值语义 + noexcept 移动"（int/string/vector…） | **不需要**——std::swap 已经 noexcept（C++11 起按成员特性条件 noexcept） |
| pImpl / 句柄 / 资源包装（本质是"指针类"） | **需要**——成员 swap 交换指针，O(1) 且天然 noexcept |
| 深存储大对象（数组内嵌/栈上 buffer） | 考虑——移动若可能分配则 swap 可能抛，noexcept 承诺要诚实 |
| 标准容器适配（放进 unordered_map 作键等） | 按 std::swap 特化规则处理（见下） |

**std::swap 特化的规则**：可以给你的类型特化 `std::swap<MyType>`——
但 C++11 起的共识是**优先②（非成员 swap + ADL）**，特化只留作兼容老代码的手段。
（全特化标准模板是允许的；但"部分特化函数模板"不存在，重载 `std::swap` 是严格禁止的——
往 `std` 里加非特化重载 = UB。）

## 4. noexcept 的诚实原则

swap 标 noexcept 前问自己：交换过程**有任何一步可能分配/抛异常吗**？
- 只交换指针/整数/平凡成员 → 放心标
- 成员里有 string/vector 等**可能分配**的类型 → 别标（它们的移动虽然 noexcept，
  但你的 swap 语义若涉及深拷贝就不成立）
- 标了 noexcept 又抛 → `std::terminate`，比不标更糟（→ 18.1 ② noexcept 语义）

## HFT 关联

- **copy-and-swap 是风控状态对象强保证的标准做法**：改配置先拷贝副本改完再
  noexcept swap 提交——热路径读旧配置零锁（配 shared_ptr/RCU 读取，→ M3 并发）
- pImpl 类型（连接句柄/会话对象）的成员 swap 是 O(1) 指针交换：
  这也是 item22/19.4 说的"冷路径 pImpl"能享受 swap 红利的原因
- `noexcept(swap(a, b))` 的 static_assert 该进你所有资源包装类的验收清单——
  vector 扩容、移动语义、copy-and-swap 三处的性能/正确性都押在这个 noexcept 上

## 代码自测

**题目 1：** 为什么 copy-and-swap 能提供强异常保证？swap 的 noexcept 在其中扮演什么角色？

<details>
<summary>参考答案</summary>

```cpp
T& operator=(T tmp) { swap(tmp); return *this; }   // 参数按值传入 = 先拷贝
```

拷贝发生在**参数构造**阶段：抛异常时原对象分毫未动（强保证的前半）。
之后的 swap 只做指针/资源过户——**它必须不抛**，否则对象处于"换了一半"的状态，
强保证崩塌。所以：拷贝可以抛（反正还没动原对象），swap 绝不可以抛——
noexcept 的 swap 是强保证的最后一环。

</details>

**题目 2：** `std::swap(a, b)` 和 `using std::swap; swap(a, b);` 在泛型代码里有什么区别？

<details>
<summary>参考答案</summary>

前者**写死**调用标准版——你的自定义 swap（非成员、靠 ADL 发现）被完全跳过，
pImpl 类型退化成三次移动（可能抛、可能慢）。
后者把 `std::swap` 引入作**兜底**，让 ADL 先找类型所在命名空间的自定义 swap：
自定义优先、标准兜底（本机实测：ADL 命中自定义版）。
泛型代码里永远写后者——这也是 18.2 ⑤ 说的"using + 未限定调用"两步走。

</details>

**题目 3：** 能不能给 `std::swap` 加一个针对 `Widget` 的重载（overload）？

<details>
<summary>参考答案</summary>

**不能**——往命名空间 `std` 里添加**重载**是未定义行为（标准只允许全特化
[full specialization] 少数模板）。两个合法选择：
① 在你的命名空间提供非成员 `swap(Widget&, Widget&)`（ADL 命中，C++11 起推荐）；
② 全特化 `template<> void std::swap<Widget>(Widget&, Widget&)`（兼容老代码的做法）。
注意重载 ≠ 特化：`namespace std { void swap(Widget&, Widget&); }` 是重载——UB。

</details>
