# 条款 29：实现引用计数智能指针，理解循环引用的来源

## 本节讲什么

**Implement a reference-counted smart pointer (the shared_ptr idea).**
承接 item28 的 unique_ptr：当所有权必须**共享**（多个持有者，最后走的释放），
引用计数是经典答案。手写教学版讲清三块：控制块、计数的原子性、
以及**循环引用**——shared_ptr 最著名的陷阱就藏在这里。

← 上一条 [item28 手写 unique_ptr](./item28-手写基础版智能指针，理解智能指针的核心逻辑.md)；
下一条 [item30 代理类](./item30-代理类（ProxyClass）设计模式，解决运算符重载、容器下标等语法痛点.md)。

---

## 1. 最小教学实现（控制块 + 计数）

```cpp
template <typename T>
class SharedPtr {
    T* ptr_ = nullptr;
    size_t* count_ = nullptr;              // 控制块：所有 SharedPtr 共享一份计数

    void release() {
        if (count_ && --*count_ == 0) {    // 最后一个持有者：
            delete ptr_;                   //   释放对象
            delete count_;                 //   释放控制块
        }
    }
public:
    explicit SharedPtr(T* p = nullptr) : ptr_(p), count_(p ? new size_t(1) : nullptr) {}
    ~SharedPtr() { release(); }

    // 拷贝 = 共享所有权：计数 +1（与 unique_ptr 的禁拷贝根本对立）
    SharedPtr(const SharedPtr& o) : ptr_(o.ptr_), count_(o.count_) {
        if (count_) ++*count_;
    }
    SharedPtr& operator=(const SharedPtr& o) {
        if (this != &o) { release(); ptr_ = o.ptr_; count_ = o.count_; if (count_) ++*count_; }
        return *this;
    }
    SharedPtr(SharedPtr&& o) noexcept : ptr_(o.ptr_), count_(o.count_) {
        o.ptr_ = nullptr; o.count_ = nullptr;    // 移动：计数不变，只是换人持有
    }
    size_t use_count() const { return count_ ? *count_ : 0; }
};
```

**控制块的意义**：计数不能放在对象里（对象不知道被谁持有）、
不能放在 SharedPtr 里（每个实例一份就各数各的）——
必须是**独立的共享区域**（所有持有者共同指向）。
这就是 std::shared_ptr 的"控制块"：refcount + weak_count + deleter + allocator。

## 2. 计数的原子性（多线程的生死线）

教学版的 `++*count_` / `--*count_` 是**普通整数操作**——多线程下
两个线程同时拷贝/析构：计数撕裂（→ 03.6 ch4 数据竞争的实测同款）。

std::shared_ptr 的计数是**原子操作**（`atomic<size_t>` 的 fetch_add/fetch_sub）——
这就是为什么 shared_ptr 的拷贝/析构有**恒定的小开销**（原子 RMW，
~10-20ns，核间竞争时更贵，→ 06.6.5 ch06）。
**热路径推论**：shared_ptr 的拷贝是原子操作——每 tick 拷几百次就是
真实的延迟项（→ HFT 关联）。

## 3. 循环引用：shared_ptr 最著名的陷阱

```cpp
struct Node {
    std::shared_ptr<Node> next;
    ~Node() { std::cout << "析构\n"; }
};
{
    auto a = std::make_shared<Node>();
    auto b = std::make_shared<Node>();
    a->next = b;            // b 的计数 = 2（b 自己 + a->next）
    b->next = a;            // a 的计数 = 2（a 自己 + b->next）
}   // 作用域结束：a、b 的局部 SharedPtr 析构——计数各减 1，
    // 但还剩 1（对方成员持有）——**永远到不了 0：两个对象双双泄漏**
```

**机制**：循环里每个对象的存活都依赖"对方先死"——死锁在内存管理上的复刻。

**解法：`weak_ptr` 破环**——

```cpp
struct Node {
    std::shared_ptr<Node> next;      // 拥有关系：强引用
    std::weak_ptr<Node> prev;        // 观察关系：弱引用（不计数）
};
// prev.lock() → 临时 shared_ptr（使用时才升级为强引用；
// 对象已死则 lock 返回空——弱引用不挡析构）
```

**判别口诀**：图里有环吗？环里至少有一条边必须 weak——
选"观察/反向"的那条（父→子强、子→父弱；拥有→强、缓存/观察→弱）。

## HFT 关联

- **热路径避 shared_ptr**：拷贝的原子 RMW + 控制块间接 + 析构不确定性
  （最后一个持有者在哪条线程不确定，释放时机不可控——
  延迟预算容不下）——热路径所有权优先 unique_ptr/池化（→ item28/18）
- shared_ptr 的正当位置在**冷路径共享**：配置快照（copy-and-swap 换配置后
  老读者的 shared_ptr 持有旧版自然释放——RCU 语义的廉价实现，
  → Effective item29 HFT 关联）
- 循环引用的真实高发区：**回调注册**（引擎持有策略、策略持有引擎）——
  策略对引擎必须 weak_ptr，否则引擎和策略互相续命到进程结束

## 代码自测

**题目 1：** 引用计数为什么不能放在对象里或 SharedPtr 里？

<details>
<summary>参考答案</summary>

放对象里：对象**不知道自己被谁持有**——而且要求对象类配合改造
（侵入式引用计数，boost::intrusive_ptr 是这个路线的合理实现，
但通用方案不能要求所有 T 都内嵌计数）。
放 SharedPtr 实例里：每个实例一份计数，各数各的——
a、b 共享同一对象但 a 的计数是 1、b 的也是 1，
a 析构时计数到 0 把对象删了，b 变野指针。
所以计数必须在**独立的控制块**——所有持有者共同指向的共享区域
（std 版的控制块还顺带装 deleter/weak_count/allocator，
make_shared 甚至把控制块和对象**一次分配**（内存局部性 + 少一次 new）——
代价是对象内存要等 weak_ptr 也清零才归还）。

</details>

**题目 2：** 循环引用为什么必然泄漏？weak_ptr 破环的选择标准是什么？

<details>
<summary>参考答案</summary>

环里每个对象的存活计数 ≥1 都来自**环内其他对象**——
作用域/容器释放外部引用后，计数各剩 1（环内互相持有），
永远到不了 0，整环泄漏（a 等 b 死、b 等 a 死——内存管理的死锁）。
weak_ptr 破环标准：**环里"非拥有"的那条边改弱**——
判别谁是 owner：父持子（父是 owner）→ 子持父用 weak；
拥有关系强、观察/缓存/回调关系弱。
例：引擎持有策略（强，引擎管策略生死）+ 策略回调引擎（弱，
引擎死了策略的 weak 自然失效，lock 返回空安全降级）。
写代码时的自检：画出拥有关系图，**环上必有一 weak**——
找不到那条该 weak 的边，就是设计还没想清楚。

</details>

**题目 3：** 为什么热路径该避开 shared_ptr 的拷贝？它的真实成本是什么？

<details>
<summary>参考答案</summary>

shared_ptr 拷贝 = **原子引用计数加一**（`atomic::fetch_add`，
lock 前缀的 RMW 指令，~10-20ns 独占 cache line 时，
核间竞争时随争用恶化——同一控制块被多核同时拷贝/析构，
cache line 在核间弹跳，→ 06.6.5 ch06 的 MESI 成本）。
析构对称：原子减一，减到零还要走控制块 + 对象的两次释放。
热路径的账：每 tick 拷几十上百次 shared_ptr =
每 tick 几十次原子 RMW + 潜在的 cache line 争用——
且**释放时机不可控**（最后一个持有者在哪条线程、哪个时刻不确定，
析构成本随机落在某个 tick 上——延迟预算的天敌）。
对照：unique_ptr 拷贝不存在（编译期禁止），移动是指针赋值零原子——
所有权语义里唯一适合热路径的形态（→ item28）。

</details>
