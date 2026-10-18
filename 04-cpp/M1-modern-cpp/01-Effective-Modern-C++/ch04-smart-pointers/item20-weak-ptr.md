# Item 20：用 std::weak_ptr 指向可能悬垂的 shared_ptr

> 第 4 章 智能指针 · Item 20 · 上一节：[Item 19 shared_ptr](item19-shared-ptr.md)

## 为什么要学这个（先建立直觉）

C 程序员用裸指针"观察但不拥有"：

```c
struct Widget* cached = NULL;
void use_cache() {
    if (cached) cached->do_work();  // 若 cached 已被 free → 悬垂指针 → UB
}
```

问题：C 的裸指针**不知道目标是否还活着**。`cached` 非空不代表指向的内存有效——用就是 UB，不用又没法检测。这类 bug 在长时运行系统里最致命：指针 99% 的时间指向有效内存，只有对象被释放后的那一下访问才崩。

C++ 的 `weak_ptr` 把"观察但不拥有"做成了可检测的语言设施：不增加强引用计数（不延长对象生命），但能通过 `lock()` **原子地**回答"对象还活着吗"——活着就给你一个 `shared_ptr` 保证使用期间不死，死了就给你空指针，走降级路径而不是 UB。

另一个容易忽视的问题：`shared_ptr` 的引用计数**解决不了环**——两个对象互相持有对方的 `shared_ptr`，谁也放不掉谁，内存永久泄漏。`weak_ptr` 是标准的破环工具。

---

## 这节讲什么

`weak_ptr` 观察 `shared_ptr` 但不拥有：不增强引用计数、不能直接解引用，访问前必须 `lock()` 提升为 `shared_ptr`（原子地检查+获取）。三大用途：打破环引用、观察者模式、缓存。

---

## 循环引用：shared_ptr 的死穴

两个结构体互相持有对方的 `shared_ptr`，本机实测（clang 23.1.0, `-std=c++17 -Wall`）：

```cpp
#include <memory>
#include <iostream>
struct B;
struct A {
    std::shared_ptr<B> b;
    ~A() { std::cout << "~A()\n"; }
};
struct B {
    std::shared_ptr<A> a;
    ~B() { std::cout << "~B()\n"; }
};

int main() {
    std::cout << "--- 进入作用域 ---\n";
    {
        auto pa = std::make_shared<A>();
        auto pb = std::make_shared<B>();
        pa->b = pb;
        pb->a = pa;
        std::cout << "pa.use_count() = " << pa.use_count() << "\n"
                  << "pb.use_count() = " << pb.use_count() << "\n";
    }
    std::cout << "--- 离开作用域（注意：没有任何析构打印）---\n";
}
```

实测输出（两个 `~` 析构打印都没出现）：

```
--- 进入作用域 ---
pa.use_count() = 2
pb.use_count() = 2
--- 离开作用域（注意：没有任何析构打印）---
```

计数推演：`pa` 销毁，A 的计数 2→1（还剩 `pb->a`）；`pb` 销毁，B 的计数 2→1（还剩 `pa` 尸体里的 `b`）。**A 的释放依赖 B 先析构，B 的释放依赖 A 先析构**——死锁式的计数环。

LeakSanitizer 实测（`-fsanitize=address -g`，`ASAN_OPTIONS=detect_leaks=1`）：

```
==84763==ERROR: LeakSanitizer: detected memory leaks
Indirect leak of 40 byte(s) in 1 object(s) allocated from:
    ... #9 in main emcC_item20_cycle.cpp:18
Indirect leak of 40 byte(s) in 1 object(s) allocated from:
    ... #9 in main emcC_item20_cycle.cpp:17
SUMMARY: AddressSanitizer: 80 byte(s) leaked in 2 allocation(s).
```

40 字节一块、共两块——正是 A、B 各自"对象+控制块"的合并分配（`make_shared` 的缘故，见 [Item 21](item21-make-functions.md)）。macOS 上 ASan **默认不开泄漏检测**，要加 `ASAN_OPTIONS=detect_leaks=1`。


### weak_ptr 破环

把环上的一条边改成 `weak_ptr`（通常选"子指父"或"回指"的那条边）：`struct B { std::weak_ptr<A> a; ... };`，本机实测输出：

```
--- 进入作用域 ---
pa.use_count() = 1        ← weak_ptr 不增强引用计数
pb.use_count() = 2
observer.expired() = 0
~A()                      ← pa 销毁时 A 立即析构
~B()                      ← A 析构 → b 成员销毁 → B 计数归零 → B 析构
--- 离开作用域 ---
observer.expired() = 1    ← 外部观察者正确看到"已失效"
lock() 返回空? 1
```

关键事实：**`weak_ptr` 不增加强引用计数**（`pa.use_count()` 保持 1），析构链因此能正常走完：`pa` 销毁 → A 计数归零 → A 析构 → 成员 `b` 销毁 → B 计数归零 → B 析构。

---

## lock() 的原子性与 expired() 的局限

`lock()` 在一次原子操作里完成：检查强引用计数 > 0 → 递增 → 返回有效 `shared_ptr`；否则返回空。为什么必须原子？看反面教材：

```cpp
if (!wp.expired()) {   // 检查
    // ← 另一线程在此刻 reset 了最后一个 shared_ptr，对象销毁
    auto p = wp.lock();  // 再获取——可能返回空！
    p->do_work();        // 空指针解引用
}
```

`expired()` 的返回值**只能用于日志和统计**——它回答的是"刚才那一刻死没死"，不是"下一秒还活着吗"。正确姿势永远是用 `lock()` 的返回值判断：`if (auto p = wp.lock()) { p->do_work(); }`——要么拿到活的，要么拿到空，没有中间态；且 `p` 持有强引用，使用期间对象保证不死。

---

## 三大经典场景

| 场景 | 谁持有 weak_ptr | lock() 失败时怎么办 |
|------|----------------|-------------------|
| 打破环引用 | 环上的"反向"边（子→父、prev） | 不会出现（环内保证存活） |
| 观察者模式 | 分发器持有观察者的 weak | 跳过并顺手清理 |
| 缓存 | 缓存表持有值的 weak | 重新加载，写回缓存 |

缓存场景的精妙之处：缓存**不阻止**对象被回收——没人用的数据自然释放，内存压力下缓存自动收缩；`lock()` 失败了再加载一次即可。这是裸指针缓存做不到的行为。

---

## 完整可运行例子

观察者 + 缓存 + expired 检测三合一，本机实测（clang 23.1.0, `-std=c++17 -Wall`）：

```cpp
#include <memory>
#include <iostream>
#include <map>
#include <string>
#include <vector>

struct Observer {
    std::string name;
    explicit Observer(std::string n) : name(std::move(n)) {}
    void update(const std::string& m) { std::cout << "  [" << name << "] 收到: " << m << "\n"; }
    ~Observer() { std::cout << "  ~Observer(" << name << ")\n"; }
};

struct Subject {
    std::vector<std::weak_ptr<Observer>> observers;
    void subscribe(const std::shared_ptr<Observer>& o) { observers.push_back(o); }
    void notify(const std::string& msg) {
        for (auto it = observers.begin(); it != observers.end();) {
            if (auto sp = it->lock()) { sp->update(msg); ++it; }
            else { std::cout << "  (跳过已销毁的观察者)\n";
                   it = observers.erase(it); }       // 顺手清理
        }
    }
};

struct Quote { double bid, ask; };
std::map<std::string, std::weak_ptr<Quote>> cache;

std::shared_ptr<Quote> getQuote(const std::string& sym) {
    if (auto sp = cache[sym].lock()) { std::cout << "  缓存命中: " << sym << "\n"; return sp; }
    std::cout << "  缓存未命中，重新加载: " << sym << "\n";
    auto sp = std::make_shared<Quote>(Quote{100.25, 100.30});
    cache[sym] = sp;
    return sp;
}

int main() {
    std::cout << "== 观察者 ==\n";
    Subject subj;
    {
        auto o1 = std::make_shared<Observer>("策略A");
        auto o2 = std::make_shared<Observer>("策略B");
        subj.subscribe(o1);
        subj.subscribe(o2);
        subj.notify("行情快照#1");
    }   // o1、o2 在此销毁
    subj.notify("行情快照#2");

    std::cout << "== 缓存 ==\n";
    {
        auto q1 = getQuote("AAPL");
        auto q2 = getQuote("AAPL");
        std::cout << "  q1.use_count() = " << q1.use_count() << " (weak_ptr 不计入)\n";
    }
    auto q3 = getQuote("AAPL");   // 缓存里只剩悬垂 weak → 重新加载

    std::cout << "== expired 检测 ==\n";
    std::weak_ptr<Quote> wq = q3;
    std::cout << "  存活时 expired = " << wq.expired() << "\n";
    q3.reset();
    std::cout << "  释放后 expired = " << wq.expired() << "\n";
}
```

实测输出：

```
== 观察者 ==
  [策略A] 收到: 行情快照#1
  [策略B] 收到: 行情快照#1
  ~Observer(策略B)
  ~Observer(策略A)
  (跳过已销毁的观察者)
  (跳过已销毁的观察者)
== 缓存 ==
  缓存未命中，重新加载: AAPL
  缓存命中: AAPL
  q1.use_count() = 2 (weak_ptr 不计入)
  缓存未命中，重新加载: AAPL
== expired 检测 ==
  存活时 expired = 0
  释放后 expired = 1
```

---

## 常见错误（新手踩坑）

1. **环引用导致内存泄漏**。A↔B 互相 `shared_ptr`，计数永不归零（本节开头实测）。修正：环上"回指"的边改 `weak_ptr`。工程惯例：所有权方向（父→子、next）用 `shared_ptr`，观察方向（子→父、prev）用 `weak_ptr`。
2. **直接用 `weak_ptr` 访问对象**。`wp->doSomething();` 编译失败——`weak_ptr` 故意不提供 `*`/`->`，逼你先 `lock()`。这个"不便"是设计，不是缺陷。
3. **`lock()` 后不检查返回值**。`auto p = wp.lock(); p->doSomething();` 当 `wp` 已悬垂时是空指针解引用。修正：`if (auto p = wp.lock()) { ... }`。
4. **用 `expired()` 做"检查后使用"**。多线程下检查和使用之间有窗口期（TOCTOU）。永远用 `lock()` 的原子语义。

---

## HFT 关联

- **策略热卸载**：行情分发器持有策略回调的 `weak_ptr`。策略模块热更新/卸载后，分发器 `lock()` 返回空自动跳过——避免向已 `dlclose` 掉的代码段回调，这是裸指针回调表最难处理的场景。
- **行情快照缓存**：`map<string, weak_ptr<Quote>>` 做快照缓存，不活跃合约的快照自动回收，命中时零拷贝共享。
- **控制块开销**：每个 `weak_ptr` 会延长控制块寿命（对象死后控制块存活到最后一个 weak 销毁）。大对象 + 长期 weak 的内存滞留问题见 [Item 21](item21-make-functions.md)。
- **热路径别滥用**：`shared_ptr`/`weak_ptr` 的计数是原子操作，每次拷贝都是一次 atomic increment。能用 `unique_ptr` 就别上 `shared_ptr`，能保证存活期的观察就用裸指针/引用——`weak_ptr` 的价值恰恰是"无法保证存活期"的场景。

---

## 自测题

**1. `weak_ptr` 为什么不能直接解引用？`lock()` 的原子性解决了什么问题？**

<details><summary>答案</summary>
`weak_ptr` 不拥有对象，故意不提供 `*`/`->`，逼你显式处理"对象可能已死"的分支。`lock()` 在一次原子操作里完成"检查计数 > 0 → 递增 → 返回 shared_ptr"，消除 TOCTOU 竞争：拆成 `expired()` 检查 + 获取两步，中间另一线程可能释放对象。返回的 `shared_ptr` 还保证使用期间对象不死。
</details>

**2. 循环引用为什么泄漏？一句话说明计数为什么回不到零。**

<details><summary>答案</summary>
A 的释放依赖 B 先析构（B 持有 `shared_ptr<A>`），B 的释放依赖 A 先析构（A 持有 `shared_ptr<B>`）——互相等待，谁也走不到第一步。实测离开作用域后无任何析构打印，LeakSanitizer 报 80 字节泄漏。
</details>

**3. `weak_ptr` 会影响 `use_count()` 吗？破环后析构顺序如何？**

<details><summary>答案</summary>
不影响——实测 `pb->a = pa`（weak）后 `pa.use_count()` 仍为 1。破环后链式析构：`pa` 销毁 → A 计数归零 → A 析构 → 成员 `b` 销毁 → B 计数归零 → B 析构。
</details>

**4. `lock()` 返回空意味着什么？正确处理方式是什么？**

<details><summary>答案</summary>
意味着对象已析构（weak_ptr expired）。正确处理是走降级路径：重新加载（缓存）、跳过并清理（观察者）——绝不能解引用返回的空 `shared_ptr`。
</details>

**5. 为什么不能用 `expired()` 做"检查后使用"？**

<details><summary>答案</summary>
`expired()` 与随后的访问之间有窗口期：返回 false 只代表"检查那一刻活着"，另一线程下一纳秒可能就释放了最后一个 `shared_ptr`。典型 TOCTOU 竞争，`lock()` 的原子语义才安全。
</details>

**6. 观察者模式里 `notify()` 时 `lock()` 失败除了跳过还应做什么？**

<details><summary>答案</summary>
顺手把悬垂的 `weak_ptr` 从列表 `erase` 掉。否则列表随时间积累大量死 weak（每个都还占着控制块），遍历成本越来越高。
</details>

**7. 下面代码输出什么？为什么？**

```cpp
auto sp = std::make_shared<int>(42);
std::weak_ptr<int> wp = sp;
std::cout << sp.use_count() << "\n";
sp.reset();
std::cout << wp.expired() << "\n";
```

<details><summary>答案</summary>
输出 `1` 和 `1`（true）。`weak_ptr` 不增强引用计数，`use_count()` 保持 1；`sp.reset()` 后唯一强引用消失，对象析构，`wp` 变 expired。
</details>

---

## 参考与延伸

- 下一节：[Item 21 make_unique/make_shared](item21-make-functions.md)
- 回到：[第 4 章 智能指针](README.md)
