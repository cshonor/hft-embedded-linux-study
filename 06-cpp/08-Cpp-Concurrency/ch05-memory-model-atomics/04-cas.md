# 5.4 CAS（Compare-Exchange）—— 无锁的基石

> 第 5 章 · 上一节：[5.3 atomic 操作](03-atomic-ops.md) · 下一节：[5.5 原子标志同步](05-atomic-flag.md)

## 这节讲什么

CAS（Compare-Exchange）是无锁编程的核心原语。`weak` 可能虚假失败（适合循环），`strong` 不会。ABA 是 CAS 的经典问题。

---

## 核心用法

```cpp
int expected = x.load();
while (!x.compare_exchange_weak(expected, desired)) {
    // 失败时 expected 被更新为当前值，重试
}
```

- `compare_exchange_strong`：失败不虚假失败，适合只需一次尝试的场景
- `compare_exchange_weak`：可能**虚假失败**（值实际等于 expected 却返回 false），适合 while 循环（性能更好）

### ABA 问题

值从 A→B→A，CAS 以为没变过——无锁队列的经典 bug。

```
线程1 读 x=A，准备 CAS(A→C)
线程2 把 x 改成 B，再改回 A
线程1 CAS(A→C) 成功——但它不知道中间发生了变化
```

**解决**：加版本号（tagged pointer）、hazard pointer、epoch 回收。

---

## 新手要点

- **`weak` vs `strong`**：在 while 循环里用 `weak`（虚假失败只是多重试一次），在单次判断用 `strong`。
- **ABA 不是理论问题**：无锁栈/队列的 push-pop 用 CAS，ABA 会导致已弹出的节点被重复使用——内存安全问题。

---

## HFT 关联

- **CAS 自旋 vs mutex**：临界区极短（几条指令）且竞争不激烈时，CAS 自旋比 mutex 快（避免上下文切换）。高竞争下 CAS 自旋烧 CPU。
- **SPSC 无锁队列**：单生产者单消费者队列用序列号 + 原子读写，无需 CAS——比 MPMC 队列简单且快。

---

## 自测题

1. `compare_exchange_weak` 和 `strong` 的区别是什么？为什么 weak 适合循环？
2. ABA 问题是什么？在无锁队列中如何发生？
3. CAS 自旋在什么条件下比 mutex 快？什么条件下更慢？

---

## 参考与延伸

- 下一节：[5.5 原子标志同步](05-atomic-flag.md)
- 回到：[第 5 章](README.md)
