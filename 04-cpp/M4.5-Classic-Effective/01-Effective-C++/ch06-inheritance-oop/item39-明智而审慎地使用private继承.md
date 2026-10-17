# 条款 39：明智而审慎地使用 private 继承

## 本节讲什么

**Use private inheritance judiciously.** private 继承的唯一语义是
**is-implemented-in-terms-of**（以某物实现）——它是复合（item38）的表亲，
表达同一种关系，但有两个复合给不了的能力：**访问基类 protected 成员**和
**EBO（空基优化）**。本机 g++ 13.3 实测私有继承的接口隐藏与 EBO 效果。

← 上一条 [item38 复合塑模 has-a](./item38-通过复合塑模出has-a或以某物实现.md)；
下一条 [item40 慎用多重继承](./item40-明智而审慎地使用多重继承.md)。

---

## 1. private 继承的语义：只要实现，不要接口

```cpp
class Stack : private std::vector<int> {
public:
    using std::vector<int>::push_back;      // 选择性曝光（item33 手法，本机实测可用）
    using std::vector<int>::back;
    using std::vector<int>::pop_back;
    // vector 的其余接口（下标/insert/erase）全部自动变成 private——用户碰不到
};
Stack s; s.push_back(1); s.back();     // ✅ 本机实测 top=2
// s[0]; s.insert(...);                // ❌ 编译错误
```

**private 继承的三条规则**：
1. 基类的 public/protected 成员**全部**变成派生类的 private——接口不继承
2. 没有隐式向上转换：`Stack*` **不能**转 `vector<int>*`（编译器当两个类型无关）——
   明确宣告"这不是 is-a"
3. 派生类（及其友元）可以访问基类的 **protected** 成员——
   这是复合（只能碰 public）给不了的

## 2. 私有继承 vs 复合：什么时候选哪个

| 需求 | 复合（item38） | 私有继承 |
|---|---|---|
| 默认选择（用别人的 public 接口） | ✅ **首选** | |
| 需要访问基类 **protected** 成员 | ❌ 够不着 | ✅ |
| 需要 override 基类的**虚函数** | ❌（成员不是类型） | ✅ |
| **EBO**（空基类零开销，本机实测） | ❌（成员至少占 1 字节） | ✅ |
| 类型语义清晰度 | ✅ 直观 | ⚠️ 生僻，需注释 |

**决策**：默认复合；只有撞上"protected / 虚函数 override / EBO"三个硬需求之一，
才换私有继承——并在类上写注释说明为什么（这语法生僻，不给理由 reviewer 会当你写错）。

## 3. EBO：私有继承的隐藏王牌（本机实测）

```cpp
struct Empty {};
struct WithMember { Empty e; int x; };      // 本机实测 sizeof = 8（Empty 占 1 + 对齐 + int）
struct WithBase : private Empty { int x; }; // 本机实测 sizeof = 4（EBO：空基不占空间！）
```

**规则**：空类作为**成员**也必须占 ≥1 字节（成员要有独立地址）；
作为**基类子对象**则允许与派生类共享地址（零开销）。

**真实用途**：策略/比较器/分配器这类"可能为空"的自定义点——

```cpp
template <typename T, typename Compare = std::less<T>>
class SortedVec : private Compare {          // Compare 通常是无状态空类——EBO 后零成本
    std::vector<T> v_;
public:
    void insert(const T& x) {
        auto pos = std::lower_bound(v_.begin(), v_.end(), x, cmp()); 
        v_.insert(pos, x);
    }
private:
    const Compare& cmp() const { return *this; }
};
// std::set/std::map 的比较器、unique_ptr 的 deleter 都是这么省空间的
```

复合版则要为这个空对象付 1 字节 + 对齐填充（实测 8 vs 4）。

## HFT 关联

- **EBO 在缓存敏感结构里是真实收益**：订单簿条目/行情消息若带
  自定义比较器/哈希器，EBO 让"空策略"零字节——每档省 4-8 字节对齐填充，
  一档 32B 的结构体可能因此不跨界（→ 06.6.5 ch06 缓存行）
- 私有继承 + using 曝光是**自定义容器适配**的老手法（Stack 例）；
  现代 C++ 更常用复合 + 少量转发（可读性优先）——知道私有继承能干什么，
  但默认写复合
- `std::unique_ptr<T, Deleter>` 的 deleter 用 EBO 存放——
  `unique_ptr<T, EmptyDeleter>` 与 `unique_ptr<T>` 同尺寸（8 字节）：
  零成本抽象的经典案例，自定义池 deleter 时同样受益

## 代码自测

**题目 1：** 私有继承和复合（item38）都表达"以某物实现"，它们的三个实质区别是什么？

<details>
<summary>参考答案</summary>

① **protected 访问**：私有继承能碰基类的 protected 成员，复合只能碰 public；
② **虚函数 override**：私有继承下派生类可以 override 基类虚函数
（基类的多态机制仍可用），复合做不到（成员对象的行为是固定的）；
③ **EBO**：空基类零开销（本机实测 sizeof 4 vs 8），复合的空成员至少占 1 字节 + 对齐。
反方向的区别：私有继承没有向上转换（明确拒绝 is-a），且语法生僻——
所以默认复合，撞上这三个硬需求才换私有继承。

</details>

**题目 2：** 为什么空类作为成员必须占空间，作为基类却可以零开销？

<details>
<summary>参考答案</summary>

C++ 的对象同一性规则：**每个完整对象必须有独一无二的地址**——
成员是完整对象，两个 Empty 成员的地址必须不同，所以至少 1 字节
（再加对齐填充，实测拖大整个结构）。
**基类子对象**不是完整对象（它是派生对象的一部分），规则允许它与派生对象
共享地址——空基类没有数据要存，共享地址后开销为零（EBO）。
本机实测：`WithMember` 8 字节 vs `WithBase` 4 字节——
差的就是 Empty 成员的 1 字节 + 对齐填充。

</details>

**题目 3：** `std::unique_ptr<T, D>` 为什么能在 deleter 为空类时保持 8 字节（和裸指针一样大）？

<details>
<summary>参考答案</summary>

因为 deleter 是用 **EBO** 存放的：libstdc++/libc++ 的实现把
`unique_ptr` 写成（概念上）`class unique_ptr : private D { T* ptr_; }` 形态——
D 是空类（无状态 deleter，如 `std::default_delete`）时基类子对象零开销，
整个 unique_ptr 只剩 `T*` 的 8 字节。
推论：给 unique_ptr 配**有状态** deleter（带 pool 指针/配置）时，
尺寸会涨到 16 字节——热路径大量存储 unique_ptr 时，
"deleter 无状态化"（状态外移到池单例/全局）是真实的尺寸优化。

</details>
