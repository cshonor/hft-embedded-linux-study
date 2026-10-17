# 条款 22：成员变量一律声明为 private

## 本节讲什么

**Declare data members private.** 这条看起来是"教科书洁癖"，实际是三条硬核工程收益：
**接口一致性、精细访问控制、实现替换自由**。还要破除两个迷思：
`protected` 并不比 `public` 好多少；"POD 全 public"在**协议/缓存行结构体**里是合理例外——
但要分清"类"和"数据包"的界限。

← 上一条 [item21 必须返回对象时别返回引用](./item21-必须返回对象时，不要强行返回引用.md)；
下一条 [item23 优先非成员非友元函数](./item23-优先用非成员、非友元函数替代成员函数.md)。

---

## 1. 三条硬核理由

### ① 接口一致性：全是函数，没有例外

```cpp
class Price {
    int64_t ticks_;             // private：内部表示（tick = 0.01 元）
public:
    int64_t ticks() const { return ticks_; }        // 读
    void set_ticks(int64_t t) { ticks_ = t; }       // 写
    double yuan() const { return ticks_ / 100.0; }  // 派生读——表示可换
};
// 用户代码永远写 p.ticks()——将来内部换成 double、换成定点库，用户代码一行不改
```

成员变量一旦 public，用户代码里就散落了 `p.ticks_`——**内部表示被用户代码绑架**，
换表示 = 全库重改重编。

### ② 精细访问控制：读写分离只有函数能做

```cpp
class Order {
    uint64_t order_id_;
public:
    uint64_t id() const { return order_id_; }       // 只读：不给 setter
    // 数量可改、价格冻结、状态只能经特定迁移——每种成员一种策略
};
```

public 成员变量只有"完全开放"一档；函数可以给出只读/只写/读写/**条件写**任意组合。

### ③ 实现替换自由（封装的真正目的）

把 `ticks_` 从 `int64_t` 换成定点类型 `fp16`：private 下只需改类内实现；
public 下所有 `p.ticks_` 的读写点全部爆炸——
**封装的收益在"改实现的那天"兑现，不是写代码的那天**。

## 2. protected 并不比 public 好多少

```cpp
class Base {
protected:
    int value_;        // 对派生类完全开放
};
```

- public 成员被**所有用户代码**依赖；protected 成员被**所有派生类代码**依赖——
  改 `value_` 同样要扫全部派生类
- 派生类数量不可控时（框架/库），protected ≈ public 的破坏力
- 结论：**成员变量只有 private 一档**；要给派生类的通道，用 protected 成员**函数**

## 3. 例外：POD / 协议结构体 / 聚合体

```cpp
struct Tick {           // 纯数据包：没有不变量要维护，没有"实现"可隐藏
    int64_t  ts_ns;
    uint32_t price;
    uint32_t qty;
    uint8_t  side;
};
// 全 public 是设计意图：它就是线上字节的 C++ 投影（→ 19.5 协议结构体纪律）
```

**判别标准**：这个类型**有不变量要维护吗**？
- 有（"价格必须为正""状态机只能这么转"）→ **private + 函数守卫**
- 没有（纯数据搬运/协议投影/聚合初始化）→ **public 成员 + static_assert 钉布局**

混淆两者的后果：给 Tick 写一堆 getter/setter（噪音且阻止聚合初始化）；
或让 Order 全 public（不变量裸奔，某天出现 qty=0 的订单）。

## HFT 关联

- **协议结构体/POD 全 public 是正义**：`static_assert(is_trivially_copyable)` 守门，
  getter/setter 反而挡住 `aggregate{...}` 初始化和 memcpy（→ 19.8 守卫模式）
- **订单簿/风控对象 private 是正义**：不变量（价格档有序、仓位非负）必须经函数维护，
  一个公开的 `position_` 就是一颗资损地雷
- getter 性能焦虑是多余的：`int64_t ticks() const { return ticks_; }` 必然内联，
  与直接访问零差别——**private 的运行时成本是 0**（成本只在编译期的类型检查里）

## 代码自测

**题目 1：** 为什么 public 成员变量会破坏「接口一致性」？

<details>
<summary>参考答案</summary>

类给用户两套语法：成员变量直接 `obj.x`，成员函数 `obj.f()`——用户要记住每个成员属于哪套；
更致命的是，public 变量把**内部表示**暴露给用户代码：将来想把 `x` 改成计算值
（没有存储、现用现算）或换类型时，所有 `obj.x` 的使用点都要改。
全函数接口下，实现怎么换，用户语法不变。

</details>

**题目 2：** `protected` 成员变量在"封装强度"上和 `public` 有什么本质区别？

<details>
<summary>参考答案</summary>

**没有本质区别**——只是依赖面从"所有用户"缩小到"所有派生类"。
对库作者来说派生类同样不可控（用户随便继承），改 protected 成员同样要全库扫描。
所以本条款的结论是"成员变量**一律** private"，给派生类留的通道应该是
protected 成员**函数**（函数可以换实现，变量不能）。

</details>

**题目 3：** 协议结构体 `Tick` 全 public 与「成员变量一律 private」矛盾吗？

<details>
<summary>参考答案</summary>

不矛盾——条款管的是**有不变量要维护的"类"**，协议结构体是**无不变量的"数据包"**：
它的每个字段独立合法（价格无所谓正负，那是语义层的事），没有"实现"可隐藏
（它本身就是线上字节的投影）。判别标准：有不变量 → private + 守卫函数；
纯数据 → public + static_assert 钉布局。把订单/仓位这类有不变量的对象也全 public，
才是真正违反本条款的场景。

</details>
