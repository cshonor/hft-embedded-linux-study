# 条款 27：尽量减少类型转型（cast）

## 本节讲什么

**Minimize casting.** 转型是"我对类型系统说谎"——每一次 cast 都是一处
类型检查被关掉的窗口。本条款讲清：C 风格强转为什么危险、C++ 四种转型的
分工与禁区、以及"想转型的时刻"通常暗示的设计缺陷。本机 g++ 13.3 实测切片与
const_cast 边界。

← 上一条 [item26 延后变量定义](./item26-尽可能延后变量定义.md)；
下一条 [item28 别返回内部句柄](./item28-不要返回指向对象内部成员的句柄（指针引用）.md)。

---

## 1. C 风格强转的问题：它什么都干得出来

```cpp
Base b1 = (Base)d;      // 本机实测：悄悄切片（Derived 的 d 成员直接丢弃）
```

`(T)x` 这一个语法，编译器会**按顺序尝试** static_cast → const_cast →
static_cast+const_cast → reinterpret_cast——**其中任何一个成功就编译通过**。
你以为在做数值转换，实际可能命中 reinterpret_cast（把指针当整数）；
你以为在向下转型，实际只是切片——**没有任何一种意图被显式表达**。

C++ 四种转型把"意图"写进语法，让编译器和 reviewer 都能检查：

| 转型 | 干什么 | 禁区 |
|---|---|---|
| `static_cast<T>(x)` | 编译期已知安全的转换（数值、向上转、void* 来回） | 向下转型无检查（错就是 UB） |
| `dynamic_cast<T>(x)` | 运行期检查的向下/横向转型（RTTI） | 非多态类型；热路径（→ 18.4） |
| `const_cast<T>(x)` | **只**加/去 const | 改"本身就是 const"的对象 = UB（实测见下） |
| `reinterpret_cast<T>(x)` | 按位重解读（协议解析的日用工具） | 对齐/别名违规（→ 19.5 UB 边界） |

## 2. const_cast 的 UB 边界（本机实测）

```cpp
const int x = 42;
*const_cast<int*>(&x) = 1;                       // UB！x 本体是 const——可能被放只读段/常量折叠

int y = 42;
*const_cast<int*>(static_cast<const int*>(&y)) = 43;   // ✅ 合法：y 本体可变，只是途经 const 视图
// 本机实测输出：y=43
```

**规则**：const_cast 只能用于"对象本身可变，只是当前经过 const 引用/指针"——
合法场景几乎都是：调用一个"忘了声明 const"的老接口，或给成员函数提供
const/non-const 双版本时去重（const 版实现，non-const 版 const_cast 转发）。

## 3. 向下转型的两个陷阱（切片 + 假向下）

```cpp
Base b2 = static_cast<Base>(d);          // 切片：b2 是纯粹的 Base，Derived 部分已消失
Derived& dr = dynamic_cast<Derived&>(b2);
// 本机实测：抛 std::bad_cast——切片后的对象"骗不过" dynamic_cast
// （它的动态类型就是 Base，typeid 里没有 Derived 的痕迹）
```

**关键认知**：切片发生在**拷贝的那一刻**，不是转型那一刻——
`static_cast<Base>(d)` 构造了一个新 Base 对象，Derived 的数据当场蒸发。
之后的任何"恢复"都是徒劳。

## 4. "想转型"的设计信号（治本）

频繁想向下转型，通常是接口设计缺陷：

```cpp
// 味道：基类指针 + dynamic_cast 决定行为
if (auto* s = dynamic_cast<SnapMsg*>(m)) { ... }
else if (auto* t = dynamic_cast<TradeMsg*>(m)) { ... }

// 治本①：虚函数——把行为放回类里（一次 vtable 跳转，无 RTTI）
m->handle(engine);

// 治本②：variant + visit（类型集合封闭时，编译期分发）
std::visit(overloaded{
    [](const SnapMsg& s) { /*...*/ },
    [](const TradeMsg& t) { /*...*/ },
}, msg);
```

**例外：reinterpret_cast 在协议解析是正当职业**——
`reinterpret_cast<const Header*>(buf)` 把字节流映射成结构体，
这是零拷贝解码的基石（→ 19.5）；它的问题不在"用了转型"，而在
**对齐/别名/字节序**三个 UB 边界要人工守住（static_assert 三件套）。

## HFT 关联

- 热路径的转型预算：**reinterpret_cast（协议解析）+ static_cast（数值/定宽）** 是全部——
  dynamic_cast 的继承链遍历和 RTTI 查询不进热路径（→ 18.4）
- 协议解码的 `reinterpret_cast` 必须配：对齐检查（`alignof(Header)` 与 buf 对齐）、
  `is_trivially_copyable` 守卫（→ 19.8）、字节序转换（hton 系列，→ 03.5）
- const_cast 在**回调注册**里是常见必要恶：C 接口要求 `void*` 用户数据，
  传 const 对象进去回调再 cast 回来——合法但要把"对象本体可变"写成注释

## 代码自测

**题目 1：** C 风格强转 `(T)x` 相比 C++ 四种转型，根本缺陷是什么？

<details>
<summary>参考答案</summary>

它把 static_cast/const_cast/reinterpret_cast 的**选择权交给编译器**——
按固定顺序逐个尝试，任何一个合法就用哪个。后果：
① 意图不可见（reviewer 不知道你想干哪种转换）；
② 代码演进时 silently 换语义（T 的继承关系一变，static_cast 可能变成 reinterpret_cast 命中）；
③ 无法被 grep（想找所有 reinterpret_cast 审计时，C 强转全漏网）。
C++ 转型的啰嗦正是价值：每种意图有专属拼写。

</details>

**题目 2：** `const int x = 42; *const_cast<int*>(&x) = 1;` 为什么不是"行为良好但风格差"，而是 UB？

<details>
<summary>参考答案</summary>

`x` 的本体是 const——编译器有权把它放进**只读数据段**，或直接**常量折叠**
（所有 `x` 的使用点替换成字面量 42，连存储都没有）。const_cast 只是移除了
指针上的 const 限定，**改变不了对象本体**——对本体 const 的对象写入：
走只读段 → 段错误；走常量折叠 → 写入无处发生、读到的还是 42。
合法场景只有"对象本体可变、只是途经 const 视图"（本机实测 y=43 生效）。

</details>

**题目 3：** 热路径上要按消息类型分发，为什么 dynamic_cast 链是坏选择？给两个更好方案。

<details>
<summary>参考答案</summary>

dynamic_cast 沿继承链**运行时遍历**（MI/虚继承更慢），每次查询一次 cache miss
起步——放在每包一次的分发热点上，延迟和抖动都不可接受（→ 18.4 ③）。
更好方案：
① **虚函数分发**：`msg->handle(e)` 一次 vtable 间接跳转，O(1)；
② **协议字段查表**：行情场景用 msg_type 直接索引 handler 数组（→ 19.3 成员指针表），
间接跳转目标规律、分支预测友好；
③ 类型集合封闭时 **variant + visit**：编译期分发，连间接跳转都可能被内联掉。

</details>
