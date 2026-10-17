# 条款 52：写了 placement new 也要写 placement delete

## 本节讲什么

**Write placement delete if you write placement new.** 自定义 placement new
（带额外参数的分配形式）有一条隐藏的配对义务：**构造函数抛异常时**，
编译器会找**签名镜像的 placement delete** 来回收内存——找不到就**不回收**（泄漏）。
这条是 new/delete 配对原则最冷门也最容易漏的一环。

← 上一条 [item51 new/delete 常规](./item51-编写new和delete时需固守常规.md)；
下一章 [ch09 杂项](../ch09-miscellaneous/)。

---

## 1. 问题：构造抛异常时，内存去哪了

```cpp
// 自定义 placement new：从自己的池分配
void* operator new(std::size_t n, Pool& pool) {
    return pool.acquire(n);
}

Widget* w = new (my_pool) Widget;   // ① operator new 拿到内存
                                    // ② Widget 构造函数抛异常！
                                    // → 内存已分配但对象没建成——谁来归还 pool？
```

`new (pool) T` 的两层（→ 19.1 ①）：先调 placement new 拿内存，再在内存上构造。
第二步抛异常时，运行库会尝试**自动回收**第一步的内存——
但它不知道该调哪个释放函数，只能找**签名与 placement new 镜像**的
placement delete：

```cpp
void operator delete(void* p, Pool& pool) noexcept {   // 镜像：同样的额外参数
    pool.release(p);
}
```

**找到了** → 调用它，内存归还；**找不到** → **什么也不做，内存泄漏**。
这就是配对义务的来源：placement delete 在正常 delete 路径**永远不会被调用**——
它只为"构造抛异常"这一条异常路径存在。

## 2. 配对规则速查

| placement new 形态 | 必须配对的 placement delete | 何时被调 |
|---|---|---|
| `operator new(size_t, Pool&)` | `operator delete(void*, Pool&)` | 构造抛异常 |
| `operator new(size_t, Arena*, int)` | `operator delete(void*, Arena*, int)` | 构造抛异常 |
| 标准 placement new（`void*` 缓冲） | 语言自带（`operator delete(void*, void*)` 空操作） | ——无需你写 |
| 普通 `operator new(size_t)` | 普通 `operator delete(void*)` | 正常 delete + 异常路径 |

**额外参数的类型/个数/顺序必须完全镜像**——差一个 const/引用都匹配不上，
匹配不上就等于没写（静默泄漏）。

## 3. 类内版本的三重注意（与 item49/51 联动）

1. 类内声明 `operator new(size_t, Pool&)` 同样**遮掩**全局 placement new
   （→ item51 ③）——标准 placement（缓冲版）要用也得类内再声明
2. 类内 placement delete 也是**静态**语义（item49 ①：析构前运行，无 this）
3. 继承传播（item49 ②）：基类的 placement new/delete 对派生类同样生效，
   尺寸不一致时池假设同样受威胁

## HFT 关联

- **无锁队列/池化订单对象**的 emplace 惯用法（→ 19.1 ③ 的 ring buffer）：
  `new (&slot) Order(args...)` 用的是**标准 placement**（语言自带空 delete）——
  但带池参数的变体（`new (pool, tag) Order`）必须手写镜像 delete，
  否则构造抛异常 = 池槽位泄漏（freelist 越用越少，容量规划被静默侵蚀）
- 异常路径的泄漏最阴险：正常压测永远触发不到"构造抛异常"——
  只有生产事故当天（异常真的来了）泄漏才发生，
  复盘时 freelist 计数对不上才知有这条路径（→ 19.1 的池监控）
- 排查手法：池的 acquire/release 配对计数器（item50 ② 统计埋点）——
  acquire > release + live_objects 且差值随异常事件增长，就是这条在漏

## 代码自测

**题目 1：** `new (pool) Widget` 的构造函数抛异常后，运行库怎么决定内存是否回收？

<details>
<summary>参考答案</summary>

`new (pool) Widget` 分两步：① `operator new(size_t, Pool&)` 分配；
② 在所得内存上运行 Widget 构造函数。
② 抛异常时，运行库要回收 ① 的内存——但它不认识你的 pool 协议，
只能查找**签名与 placement new 完全镜像**的 `operator delete(void*, Pool&)`：
找到 → 调用它回收；找不到 → **放弃回收，内存泄漏**。
所以 placement delete 只为这条异常路径存在（正常 delete 永远调普通版）——
"写了 placement new 就要写 placement delete"不是风格建议，是泄漏防线。

</details>

**题目 2：** 为什么标准 placement new（`new (buf) T`）不需要你写 placement delete？

<details>
<summary>参考答案</summary>

因为语言**自带**它的镜像：标准库提供 `operator delete(void*, void*) noexcept`，
实现是**空操作**——缓冲内存不归 new 管（它是调用方的栈数组/mmap/池槽），
"回收"本来就无事可做。
构造抛异常时运行库调这个空 delete，语义正确（内存还在 buf 里，归调用方）。
只有**自定义 placement new**（带 Pool&/Arena* 等额外参数、内存由分配函数
从某处"拿出"）才需要镜像 delete——有"拿出"就要有"归还"，
语言替不了你的私有协议。

</details>

**题目 3：** 池化订单系统的监控发现 freelist 缓慢下降但无对象泄漏报告，最可能的原因是什么？

<details>
<summary>参考答案</summary>

最可能是**构造抛异常路径的 placement delete 缺失**（或签名不镜像）：
`new (pool) Order(args...)` 在构造失败时池槽位没有归还——
freelist 只减不增，但"活着的对象"计数正常（对象根本没建成），
所以泄漏检测（基于对象生命周期）看不到，只有槽位计数在流血。
验证：给池的 acquire/release 加配对计数，人为触发一次构造异常
（mock 一个会抛的构造函数），观察 acquire - release 差值是否 +1。
修复：补镜像 `operator delete(void*, Pool&) noexcept`，
静态断言两个签名的参数列表一致（→ item52 配对规则速查表）。

</details>
