# Item 20：用 std::weak_ptr 指向可能悬垂的 shared_ptr

> 第 4 章 智能指针 · Item 20 · 上一节：[Item 19 shared_ptr](item19-shared-ptr.md)

## 为什么要学这个（先建立直觉）

C 程序员用裸指针"观察但不拥有"：

```c
struct Widget* cached = NULL;

void use_cache() {
    if (cached) {
        cached->do_work();  // 如果 cached 已被 free → 悬垂指针 → UB
    }
}
```

问题：C 的裸指针不知道目标是否还活着。`cached` 可能指向已释放的内存——用就是 UB，不用又没法检测。

C++ 的 `weak_ptr` 解决了这个问题：它不增加强引用计数（不延长对象生命），但能用 `lock()` **原子地**检查"对象还活着吗"并获取 `shared_ptr`：

```cpp
std::weak_ptr<Widget> wp = sp;
if (auto p = wp.lock()) {
    p->do_work();   // 对象还活着，安全使用
} else {
    // 对象已被销毁，重新加载或跳过
}
```

---

## 这节讲什么

`weak_ptr` 不增加强引用计数，观察 `shared_ptr` 但不延长对象生命。要访问对象须 `lock()` 提升为 `shared_ptr`（原子地检查并获取）。

---

## 核心用法

### lock() 的原子性

```cpp
std::shared_ptr<Widget> sp = std::make_shared<Widget>();
std::weak_ptr<Widget> wp = sp;   // wp 观察 sp，不增加强引用计数

// 在另一个线程里 sp 可能被 reset
// sp.reset();

// 安全访问：lock() 是原子的
if (auto p = wp.lock()) {
    // p 有效（强引用计数 >= 1），安全使用
    p->doSomething();
} else {
    // 对象已被销毁，wp 是悬垂的
}
```

`lock()` 是原子的：检查强引用计数 > 0，若是则递增并返回 `shared_ptr`；否则返回空 `shared_ptr`。这避免了"检查后使用"的 TOCTOU 竞争。

### 三大用途

```cpp
// 1. 缓存：对象不使用时可被回收
std::map<Key, std::weak_ptr<Data>> cache;
auto get_data(const Key& k) {
    if (auto sp = cache[k].lock()) return sp;   // 缓存命中
    auto sp = std::make_shared<Data>(load(k));   // 重新加载
    cache[k] = sp;
    return sp;
}

// 2. 观察者模式：观察者不延长被观察对象的生命
class Subject {
    std::vector<std::weak_ptr<Observer>> observers;
public:
    void notify() {
        for (auto& wp : observers) {
            if (auto sp = wp.lock()) {
                sp->update();  // 只通知活着的观察者
            }
        }
    }
};

// 3. 打破环引用
struct Node {
    std::shared_ptr<Node> next;
    std::weak_ptr<Node> prev;  // weak 打破环！
    // 如果 prev 也用 shared_ptr → 环引用 → 内存泄漏
};
```

---

## 常见错误（新手踩坑）

**错误 1：环引用导致内存泄漏**
```cpp
struct A { std::shared_ptr<B> b; };
struct B { std::shared_ptr<A> a; };  // 环！
auto pa = std::make_shared<A>();
auto pb = std::make_shared<B>();
pa->b = pb;  // b 的引用计数 = 2
pb->a = pa;  // a 的引用计数 = 2
// pa, pb 离开作用域 → 引用计数各降 1 → 但还有 1 → 永不归零 → 内存泄漏
```
**修正：** 一方用 `weak_ptr` 破环。

**错误 2：直接用 weak_ptr 访问对象**
```cpp
std::weak_ptr<Widget> wp = sp;
wp->doSomething();  // 编译失败！weak_ptr 没有 -> 操作符
```
**修正：** 先 `lock()` 拿到 `shared_ptr` 再访问。

**错误 3：lock() 后不检查返回值**
```cpp
auto p = wp.lock();  // 可能返回空 shared_ptr
p->doSomething();    // 如果 wp 已悬垂 → 空指针解引用 → UB
```
**修正：** `if (auto p = wp.lock()) { p->doSomething(); }`

---

## 新手要点（和 C 的区别）

| 维度 | C 怎么做 | C++ 怎么做 | 为什么 |
|------|---------|-----------|--------|
| 观察指针 | 裸指针（可能悬垂） | `weak_ptr`（可检测） | 安全 |
| 悬垂检测 | 无法检测 | `lock()` 返回空 | 原子检查 |
| 引用计数 | 手动管理 | `shared_ptr` 自动 | RAII |
| 环引用 | 不适用（C 无引用计数） | `weak_ptr` 破环 | 避免内存泄漏 |

**一句话总结：** C 程序员记住——`weak_ptr` 是"能检测悬垂的观察指针"。用 `lock()` 原子地检查+获取，比 C 的裸指针安全得多。

---

## HFT 关联

- **观察者模式**：行情分发器持有策略的 `weak_ptr`，策略销毁后分发器 `lock()` 返回空自动跳过——避免策略热卸载时的悬垂回调。
- **缓存**：行情快照缓存用 `weak_ptr`，内存紧张时可自动回收，`lock()` 失败时重新拉取。
- **环引用**：双向链表/图结构用 `weak_ptr` 打破环，避免 `shared_ptr` 导致的内存泄漏。

---

## 自测题

1. `weak_ptr` 如何安全访问对象？`lock()` 的原子性为什么重要？
2. `weak_ptr` 的三大用途是什么？
3. 什么是 `shared_ptr` 的环引用问题？`weak_ptr` 如何解决？
4. `lock()` 返回空 `shared_ptr` 意味着什么？
5. 下面代码有什么问题？
```cpp
std::weak_ptr<Widget> wp;
{
    auto sp = std::make_shared<Widget>();
    wp = sp;
}
auto p = wp.lock();
p->doSomething();
```

<details>
<summary>参考答案</summary>

1. `weak_ptr` 不拥有对象、不能直接用 `*` / `->` 访问，必须先 `lock()` 得到一个 `shared_ptr`，再检查它是否为空后使用。`lock()` 的原子性至关重要：它必须在一次原子操作里完成"检查弱计数/强计数是否仍大于 0，若是则递增强引用计数"。如果这两步分开做（先 `expired()` 判断再解引用），中间可能被其他线程抢先释放对象，你拿到的就是一个已经悬垂的指针——`lock()` 保证"要么拿到一个有效的 `shared_ptr`，要么拿到空"，不存在中间态。

2. 三大用途：①**打破 `shared_ptr` 环引用**（父子、双向链表、图结构里让一方持有 `weak_ptr`）；②**观察者/回调的非拥有引用**（分发器持有观察者的 `weak_ptr`，对方销毁后 `lock()` 失败自动跳过，避免悬垂回调）；③**缓存**（缓存持有 `weak_ptr`，对象无人使用时自动释放；`lock()` 失败就重新加载，内存压力下可自动回收）。此外也用于 `enable_shared_from_this` 的内部实现。

3. 环引用指两个（或多个）对象互相持有 `shared_ptr`，形成引用计数环：A 的计数靠 B 释放、B 的计数靠 A 释放，谁也不会归零，两个对象都无法析构，造成内存泄漏（且这类泄漏在长时间运行的行情/策略系统里会持续累积）。`weak_ptr` 通过"只观察、不增加强引用计数"打破环：把环上的一条边改成 `weak_ptr`（通常是子节点指向父节点、观察者指回主体），计数就能正常归零。注意这只解决**强引用环**，若还涉及 `shared_from_this` 或自环同样需要检查。

4. 意味着被观察的对象**已经不存在了**（强引用计数已归零、对象已被析构，`weak_ptr` 处于 expired 状态）。此时必须跳过使用、走降级路径（重新加载、重新注册、忽略该观察者），绝不能解引用——`lock()` 返回的空 `shared_ptr` 解引用是空指针解引用，属于未定义行为。因此正确写法总是先判空：
```cpp
if (auto p = wp.lock()) { p->doSomething(); }
```

5. 有问题，且会崩溃。内层作用域结束时 `sp` 被销毁，`Widget` 的唯一强引用消失，对象被析构；`wp` 随之 expired。之后 `wp.lock()` 返回一个**空** `shared_ptr`，`p->doSomething()` 就是对空 `shared_ptr` 解引用——未定义行为（实际上通常是空指针调用成员函数而崩溃；若成员函数不访问 `this` 可能"碰巧"不崩，但依然非法）。修正：
```cpp
if (auto p = wp.lock()) { p->doSomething(); }
else { /* 对象已销毁，走降级路径 */ }
```

</details>

---

## 参考与延伸

- 下一节：[Item 21 make_unique/make_shared](item21-make-functions.md)
- 回到：[第 4 章 智能指针](README.md)
