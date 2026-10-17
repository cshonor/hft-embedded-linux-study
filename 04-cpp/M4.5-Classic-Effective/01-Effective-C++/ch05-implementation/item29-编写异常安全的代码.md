# 条款 29：编写异常安全的代码

## 本节讲什么

**Strive for exception-safe code.** 异常安全的代码满足两件事：**不泄漏资源**、
**不破坏数据结构**。本条款给出三级保证的工程含义与两个标准实现手法
（RAII、copy-and-swap）——这是 18.1（异常机制）的工程落地，也是 item25
（noexcept swap）存在的原因。

← 上一条 [item28 别返回内部句柄](./item28-不要返回指向对象内部成员的句柄（指针引用）.md)；
下一条 [item30 内联的里里外外](./item30-透彻理解内联inline的优缺点.md)。

---

## 1. 泄漏型 vs 破坏型（异常安全的两个敌人）

```cpp
// 敌人一：资源泄漏
void feed_handler(const Msg& m) {
    Tick* t = new Tick;
    parse(m, t);             // 抛异常 → t 泄漏
    publish(t);
    delete t;
}

// 敌人二：数据结构破坏（更可怕）
void OrderBook::insert(const Order& o) {
    levels_.push_back(o);        // 已插入
    index_[o.id] = &levels_.back();  // 若这步抛（map 分配失败）→
}                                    // 簿子里有订单，索引里查不到——不变量已死
```

泄漏型用 **RAII** 解决；破坏型要按**保证级别**设计操作顺序。

## 2. 三级保证（复习 + 工程含义）

| 级别 | 承诺 | 实现手法 | 成本 |
|---|---|---|---|
| 基本保证 | 不泄漏、不变量不破坏，但状态可能已变 | RAII + 操作顺序小心 | 低——**所有代码的底线** |
| 强保证 | 要么全成功，要么完全回滚 | **copy-and-swap**：先改副本，noexcept swap 提交 | 一次拷贝的成本 |
| 不抛保证 | 永不抛 | 全路径 noexcept（不许分配/不许抛） | 设计约束大——留给 swap/移动/析构 |

**工程结论**：**所有函数至少基本保证**；对外的"事务性"操作（配置切换、
状态提交）给强保证；swap/移动/析构必须不抛（→ item25、18.1 ②）。

## 3. 手法一：RAII——泄漏型的通解

```cpp
void feed_handler(const Msg& m) {
    auto t = std::make_unique<Tick>();   // 异常时 unique_ptr 自动释放
    parse(m, t.get());                   // 随便抛
    publish(t.get());
}                                        // 离开作用域即释放（含异常路径——栈展开，18.1 ①）
```

RAII 清单（交易系统的日常）：`unique_ptr`/`shared_ptr`（内存）、
`lock_guard`/`scoped_lock`（锁）、`fstream`（文件）、
以及自己写的守卫（网卡混杂模式开关、netem 规则——脚本层的 `trap EXIT` 是同理念）。

## 4. 手法二：copy-and-swap——强保证的标准做法

```cpp
class RiskConfig {
    struct Impl { Limits l; Throttles t; };
    std::unique_ptr<Impl> impl_;
public:
    RiskConfig& operator=(RiskConfig tmp) noexcept {   // ① 参数按值传入：拷贝在调用前完成
        swap(tmp);                                     // ② noexcept 交换：提交不可失败
        return *this;
    }
    void swap(RiskConfig& o) noexcept { std::swap(impl_, o.impl_); }   // → item25
};

// 用法：改配置
RiskConfig new_cfg = current;      // 拷贝副本（这步可以抛——原配置未动）
new_cfg.set_limit(SIDE_BUY, 1000); // 在副本上改（抛了也不影响线上）
current = std::move(new_cfg);      // noexcept 提交：要么整个生效，要么不生效
```

**强保证的分解**：可能抛的部分（拷贝/修改）在**原对象之外**完成；
不能抛的部分（swap）只是一次指针过户（item25 ② 实测）。
这就是为什么 item25 要求 swap noexcept——它是强保证的最后一环。

## 5. 异常安全的现实边界（诚实清单）

- **强保证不是免费的**：copy-and-swap 要付一次深拷贝——大对象/热路径用不起，
  退而求其次：基本保证 + 显式两阶段提交（先 prepare 校验，后 commit 只做 noexcept 操作）
- **析构/swap/移动是"不抛区"**：这三个 noexcept 失守，整个异常安全体系崩塌
- **异常 vs 错误码**：热路径用错误码（异常开销不可预测，18.1 HFT），
  冷路径用异常 + 本条手法——**异常安全主要在冷路径发光**（启动/配置/重连）

## HFT 关联

- **风控配置热更新**是本条的教科书场景：copy-and-swap + shared_ptr 读侧
  （读配置零锁：持有旧 shared_ptr，换配置换指针——RCU 思想，→ M3 并发 ch07）
- 撮合引擎的"订单插入"只给**基本保证**（插入失败可重试，回滚语义反而危险——
  已报给交易所的订单不能"撤回"），强保证用在**状态快照**（日终结算）
- 异常路径的栈展开成本（18.1 ①）= 每帧的析构清单——RAII 对象越多展开越贵，
  这是"热路径零异常"的另一层理由

## 代码自测

**题目 1：** 下面代码违反的是哪一级保证？

```cpp
void OrderBook::insert(const Order& o) {
    levels_.push_back(o);
    index_[o.id] = &levels_.back();   // map 分配失败抛异常
}
```

<details>
<summary>参考答案</summary>

**基本保证**——`push_back` 成功后 `index_` 插入失败：
簿子里有订单但索引查不到，**不变量已破坏**（数据不一致，不只是泄漏）。
修法取决于承诺级别：
基本保证版——先插索引（可抛的操作放前面？不行，引用还没稳定）……
实际上正确做法是**先完成所有可能抛的准备工作再提交**：
`auto* slot = &levels_.emplace_back(o); try { index_[o.id] = slot; }
catch (...) { levels_.pop_back(); throw; }`——手动回滚到一致状态；
或重构为单数据源（索引只存 id→序号，插入一体完成）。
核心原则：**可能抛的操作要么全在"提交点"之前，要么配回滚**。

</details>

**题目 2：** copy-and-swap 的 `operator=` 为什么参数按值传入？`noexcept` 标在哪一步？

<details>
<summary>参考答案</summary>

`T& operator=(T tmp)` 按值传入 = **拷贝构造在函数体外（调用方侧）完成**——
那一步抛异常时原对象分毫未动（强保证的前半）。
进入函数体后只剩 `swap(tmp)`——指针过户，必须 `noexcept`（强保证的后半，
→ item25 ② 实测 `static_assert(noexcept(swap(a,b)))`）。
所以 noexcept 标在 swap 和 operator= 上；**不能**标在拷贝构造上（它就是可能抛的那步）。

</details>

**题目 3：** 为什么热路径的风控检查不用"异常 + 强保证"，而是用"错误码 + 两阶段"？

<details>
<summary>参考答案</summary>

① 异常成本不可预测：throw 要查 LSDA 表 + 逐帧栈展开（µs 级且抖动大）——
tick-to-trade 预算里没有它的位置（18.1 HFT）；
② 强保证的 copy-and-swap 要付**深拷贝**——风控状态大，每笔检查拷一次不可行；
③ 风控检查本质是**只读判定**（超限就拒单）——没有"改一半"的问题，
基本保证都谈不上，直接返回错误码最诚实。
两阶段模式留给"必须改状态"的场景：prepare（校验/分配，可失败）+
commit（纯指针交换，noexcept）——把强保证的语义保留，把拷贝的成本砍掉。

</details>
