# 条款 18：通过重载 operator new 实现自定义内存池

## 本节讲什么

**Implement a custom memory pool via operator new overloading.**
承接 item17 的量化判别：当池化被证明必要，这就是最小可运行实现——
固定尺寸 freelist 池 + 类内 operator new/delete（→ item49 的机制、
19.1 的两层模型、item51/52 的常规合规）。
本节代码为教学骨架，可直接编译运行。

← 上一条 [item17 new/delete 开销](./item17-深度剖析newdelete底层内存分配开销，优化堆内存使用.md)；
下一条 [item19 RVO/NRVO 复习](./item19-理解临时对象、拷贝构造、返回值优化RVONRVO.md)。

---

## 1. 最小可运行实现（固定尺寸池）

```cpp
#include <cstddef>
#include <new>

template <typename T, std::size_t N>
class FixedPool {
    union Slot {                            // union：空闲时存 next 指针，
        Slot* next;                         // 占用时存对象——零元数据开销
        alignas(T) char storage[sizeof(T)];
    };
    Slot arena_[N];                         // 预分配连续内存（缓存友好，item17 ③）
    Slot* free_ = nullptr;
public:
    FixedPool() {                           // 启动时串成 freelist
        for (std::size_t i = 0; i + 1 < N; ++i) arena_[i].next = &arena_[i + 1];
        arena_[N - 1].next = nullptr;
        free_ = &arena_[0];
    }
    void* acquire() noexcept {
        if (!free_) return nullptr;         // 池空 = 显式失败（item49 ③ 的语义选择）
        Slot* s = free_; free_ = free_->next;
        return s;
    }
    void release(void* p) noexcept {        // noexcept 铁律（→ MoreEff item11）
        if (!p) return;                     // nullptr 免疫（item51 常规）
        auto* s = static_cast<Slot*>(p);
        s->next = free_; free_ = s;         // 头插归还，O(1) 无锁
    }
};

// 用法：类内重载接池（→ item49 的形态）
class Order {
    inline static FixedPool<Order, 4096> pool_;   // C++17 inline 静态成员
public:
    static void* operator new(std::size_t) {
        if (void* p = pool_.acquire()) return p;
        throw std::bad_alloc();             // 或按容量语义 terminate（item17 HFT）
    }
    static void operator delete(void* p) noexcept { pool_.release(p); }
    static void operator delete(void* p, std::size_t) noexcept { pool_.release(p); }
    // ...订单字段...
};
```

## 2. 骨架的五个设计点（每处都对应一条已学条款）

| 设计 | 对应条款 |
|---|---|
| union Slot：空闲槽位复用为 next 指针 | 零元数据（→ 19.1 sized delete 的同一哲学） |
| arena 预分配连续数组 | 缓存局部性（→ item17 ③）+ 一次 malloc（无碎片） |
| acquire/release 全 noexcept + nullptr 免疫 | MoreEff item11 + Effective item51 常规 |
| 池空返回 nullptr → 调用方决定语义 | item49 ③：容量事故显式化，不静默降级 |
| `operator delete` 双版本（含 sized） | item51 常规 + 19.1 ② sized delete 红利 |

## 3. 生产化还差什么（诚实清单）

教学骨架 → 生产池的距离：
1. **线程安全**：多线程域加锁 / per-CPU 池 / 无锁 freelist（CAS + ABA 防护，→ M3 并发 ch07）
2. **prefault/mlock**：启动触碰全部页（→ item17 ③），把页错误付在启动期
3. **统计埋点**：acquire/release 计数 + 高水位（→ Effective item50 ② + item52 的泄漏侦测）
4. **构造/析构边界**：池只给裸内存——`new (slot) T(...)` placement 构造
   + 手动析构归还（→ 19.1 ③ 铁律），类内 operator new 形态已含此语义
5. **容量规划**：N 从哪来（峰值 × 余量）+ 池空告警（→ item48 预案哲学）

## HFT 关联

- 这就是"运行期零分配"的最小形态：订单/回报对象每 tick 数千次
  acquire/release 全部 O(1) 无锁、常数时间——malloc 从热路径彻底消失
- 池类型按**尺寸档**组织（Order 一池、ExecutionReport 一池）——
  避免可变尺寸的碎片问题（freelist 池只服务固定尺寸，→ item17 ③）
- 无锁化升级路径：单线程域（一个线程一个池，免去同步）→ per-CPU 池 →
  MPMC 无锁 freelist（→ 06.6.5 无锁专题）——按真实并发度逐级加码，
  别一上来就 CAS（正确性成本指数级）

## 代码自测

**题目 1：** union Slot 为什么能省掉元数据？代价是什么？

<details>
<summary>参考答案</summary>

union 让同一块内存在两种身份间复用：**空闲时**存 `next` 指针
（串 freelist），**占用时**存对象本身——空闲槽位不需要任何额外元数据
（malloc 每块要 8-16B 头部存尺寸/链表指针，池的这个开销是零）。
代价：① 只适合**固定尺寸**（每槽一样大，可变尺寸要分档）；
② 类型擦除——调试时"这个槽里是什么"没有记录
（生产化时用 debug 版加元数据，或按 item50 ② 的统计埋点补偿）；
③ 对齐必须手工保证（`alignas(T)`——union 成员取最大对齐，写错就是 UB，→ 19.5）。

</details>

**题目 2：** 池空时 `acquire()` 返回 nullptr，`operator new` 为什么抛 bad_alloc
而不是返回 nullptr？这个选择背后的语义是什么？

<details>
<summary>参考答案</summary>

`operator new` 的语言常规（Effective item51 ④）：失败必须抛 `std::bad_alloc`
（或调 new-handler）——返回 nullptr 违反常规
（用户代码 `new Order` 后不做判空检查，null 被解引用就是 UB）。
但更深的语义在 HFT 语境（item17 ③ / item49 ③）：
**池空 = 容量规划事故**——订单池在盘中耗尽意味着"峰值估算错了"或
"对象泄漏了"（→ item52 的泄漏侦测），两者都是必须**当场暴露**的事故，
不能静默继续（返回 nullptr 让错误以"随机 crash"的形式在下游爆发）。
教学版抛 bad_alloc 是守语言常规；生产版直接 terminate + 高水位告警
更接近"容量事故当场爆炸"的哲学——两条都比"返回 null 继续跑"诚实。

</details>

**题目 3：** 单线程域的池要支持"网关线程 + 策略线程"两个线程，三个升级方案的风险排序？

<details>
<summary>参考答案</summary>

从低风险到高风险：
① **每线程一池**（thread_local FixedPool）——零同步零共享，
风险最低（代价：两个池各自容量规划，池间不能互借——对象跨线程归还时
要路由回 owner 池，加 owner 标记）；
② **per-CPU 池 + 远程归还队列**——借出方线程的"待归还队列"无锁 MPSC，
均衡性更好（实现复杂度中等）；
③ **MPMC 无锁 freelist**（CAS + ABA 计数/tag）——共享最充分，
但 ABA/内存序的正确性成本指数级（→ M3 并发 ch07：无锁数据结构是
独立的专业领域，"看起来简单的 CAS 版"几乎必然有 ABA 窗口）。
原则：无共享优于少共享优于真共享——
能①不②，能②不③（→ 06.6.5 无锁专题的同一结论）。

</details>
