# Item 20：用 std::weak_ptr 指向可能悬垂的 shared_ptr

> 第 4 章 智能指针 · Item 20 · 上一节：[Item 19 shared_ptr](item19-shared-ptr.md)

## 这节讲什么

`weak_ptr` 不增加强引用计数，观察 `shared_ptr` 但不延长对象生命。要访问对象须 `lock()` 提升为 `shared_ptr`（原子地检查并获取）。

---

## 核心用法

```cpp
std::weak_ptr<Widget> wp = sp;
if (auto p = wp.lock()) {
    // p 有效，安全使用
} else {
    // 对象已被销毁
}
```

`lock()` 是原子的：检查强引用计数 > 0，若是则递增并返回 `shared_ptr`；否则返回空 `shared_ptr`。这避免了"检查后使用"的 TOCTOU 竞争。

---

## 三大用途

1. **缓存**：缓存用 `weak_ptr` 指向对象，对象不使用时可被回收，`lock()` 失败时重新加载。
2. **观察者模式**：观察者持有被观察对象的 `weak_ptr`，被观察对象销毁后观察者自动感知。
3. **打破环引用**：A↔B 互引的 `shared_ptr` 导致计数永不归零、内存泄漏。一方改用 `weak_ptr` 破环。

---

## 新手要点（和 C 的区别）

- **C 没有这个概念**：C 里"观察但不拥有"靠裸指针，但裸指针无法知道目标是否已被释放（悬垂指针 UB）。`weak_ptr` 的 `lock()` 解决了这个问题。
- **环引用**：两个对象互相用 `shared_ptr` 指向对方 → 引用计数永不归零 → 内存泄漏。这是 `shared_ptr` 最经典的坑，用 `weak_ptr` 破环。

---

## HFT 关联

- **观察者模式**：行情分发器持有策略的 `weak_ptr`，策略销毁后分发器 `lock()` 返回空自动跳过——避免策略热卸载时的悬垂回调。

---

## 自测题

1. `weak_ptr` 如何安全访问对象？`lock()` 的原子性为什么重要？
2. `weak_ptr` 的三大用途是什么？
3. 什么是 `shared_ptr` 的环引用问题？`weak_ptr` 如何解决？
4. `lock()` 返回空 `shared_ptr` 意味着什么？

---

## 参考与延伸

- 下一节：[Item 21 make_unique/make_shared](item21-make-functions.md)
- 回到：[第 4 章 智能指针](README.md)
