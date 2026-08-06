# 7.3 ABA 问题

> 第 7 章 · 上一节：[7.2 无锁栈（Treiber Stack）](02-treiber-stack.md) · 下一节：[7.4 无锁队列（SPSC）](04-spsc-queue.md)

## 这节讲什么

ABA 是无锁编程特有的经典问题：CAS 只检查"值是否相等"，但值可能 A→B→A 回到原值，CAS 误以为没变。本节讲 ABA 的成因、在栈和队列中的表现、以及四种解法（ tagged pointer / hazard pointer / epoch reclamation / 引用计数）。

---

## 核心规则（代码+表格）

### ABA 问题示例

以 Treiber Stack 的 pop 为例：

```
初始：head → A → B → C

线程1 执行 pop：
  old_head = head (A)
  next = old_head->next (B)
  --- 此时被抢占 ---

线程2 执行：
  pop() → 移除 A，head = B，delete A
  pop() → 移除 B，head = C，delete B
  push(X) → 恰好分配到 A 的旧地址（内存回收重用），head = A，A->next = C

线程1 恢复：
  CAS(head, A, B)   // head == A? yes！（A 被重用了）
                    // head = B  → 但 B 已被 delete！
  → use-after-free / 悬挂指针
```

CAS 以为"head 没变（还是 A）"，但实际上 A 被弹出过、地址被重用——这就是 ABA。

### 四种解法

| 解法 | 思路 | 优点 | 缺点 |
|------|------|------|------|
| **Tagged Pointer** | 指针 + 版本号，CAS 同时比较两者 | 简单直接 | 需 128 位 CAS（`atomic<struct{ptr,tag}>`） |
| **Hazard Pointer** | 每线程登记"正在用的指针"，回收前检查 | 安全、通用 | 每次操作要登记/清除，有开销 |
| **Epoch Reclamation** | 全局 epoch，延迟回收（两代以上才删） | 低开销 | 需要所有线程定期进入新 epoch |
| **引用计数** | 节点引用计数归零才删 | 自动回收 | 原子引用计数本身有 CAS 开销 |

### Tagged Pointer 示意

```cpp
// 指针 + 版本号打包，CAS 比较整个包
struct tagged_ptr {
    node* ptr;
    uint64_t tag;   // 每次修改 +1
};
std::atomic<tagged_ptr> head;

void push(const T& v) {
    tagged_ptr old = head.load();
    node* n = new node(v);
    tagged_ptr new_val{n, old.tag + 1};
    n->next = old.ptr;
    while (!head.compare_exchange_weak(old, new_val)) {
        new_val.tag = old.tag + 1;  // 失败：更新 tag
        n->next = old.ptr;
    }
}
// 即使地址 A 被重用，tag 不同 → CAS 失败 → 安全
```

注意：`tagged_ptr` 通常是 16 字节（8 字节指针 + 8 字节 tag），需要 `cmpxchg16b`（x86-64）指令支持。C++20 的 `std::atomic_ref` 对 16 字节结构有支持，但可能退化成用锁。

### Hazard Pointer 简化版

```cpp
// 每线程有一个 "hazard slot" 登记正在用的指针
std::atomic<node*> hazard[MAX_THREADS];

bool pop(T& result) {
    int tid = get_thread_id();
    node* old_head;
    do {
        old_head = head.load();
        hazard[tid] = old_head;     // 登记：我在用这个节点
        // ⚠️ 必须重新检查，因为 head 可能在此期间变了
    } while (old_head != head.load());  // 重读确认

    if (!old_head) return false;
    while (!head.compare_exchange_weak(old_head, old_head->next));

    result = old_head->data;
    // 回收前检查：有没有其他线程的 hazard slot 指向它？
    bool safe = true;
    for (int i = 0; i < MAX_THREADS; ++i)
        if (hazard[i].load() == old_head) { safe = false; break; }
    if (safe) delete old_head;
    else /* 放入待回收列表，稍后再试 */;
    hazard[tid] = nullptr;
    return true;
}
```

---

## 新手要点（和 C 的区别）

- **ABA 是 C 程序员从未遇到的问题**：C 的 `pthread_mutex` 保护下不存在 ABA——锁保证整个操作原子。只有无锁编程才会遇到"值变了又变回来"的问题。
- **C 程序员容易忽视内存重用**：`free` 后地址会被 `malloc` 重用——这在单线程下没问题，但在无锁下导致 ABA。C++ 的 `new`/`delete` 同理，但无锁结构必须显式处理。
- **Tagged pointer 需要 128 位 CAS**：C 里用 `__int128` + 内建函数，C++20 可用 `std::atomic<tagged_ptr>`（但实现可能退化成锁）。这在 32 位平台更难——只有 64 位 CAS。
- **Hazard pointer 概念较新**：C++ 程序员也未必熟悉。它是 Maged Michael 在 2004 年提出的，直到 C++ 标准提案（P2530）才开始标准化。目前靠手写或第三方库。

---

## HFT 关联

- **HFT 无锁结构必须解决 ABA**：HFT 热路径（行情队列、订单队列）如果用无锁结构，ABA 不解决就是定时炸弹——在高负载下内存重用频繁，ABA 触发概率不低。
- **mempool + tag 是 HFT 常用方案**：mempool 节点地址固定（不真正 delete），配合 tagged pointer 防止逻辑 ABA。mempool 的 slot 复用导致地址重用，但 tag 递增保证 CAS 能检测到。
- **Hazard pointer 在 HFT 中的开销**：每次 pop 要登记 hazard + 扫描所有线程的 slot——线程数多时扫描代价高。HFT 通常绑核且线程数固定（如 4-8 个），hazard pointer 可接受。
- **Epoch reclamation 适合 HFT**：HFT 系统所有线程定期进入新 epoch（如每 tick 或每批次），延迟回收的旧节点在下一个 epoch 后安全删除——开销低，适合批量场景。

---

## 自测题

1. 什么是 ABA 问题？为什么 CAS 检测不到 A→B→A 的变化？
2. 在 Treiber Stack 的 pop 中，ABA 如何导致 use-after-free？
3. Tagged pointer 如何解决 ABA？为什么需要 128 位 CAS？
4. Hazard pointer 的基本原理是什么？为什么登记后要"重读确认"？
5. 为什么 mempool + tag 是 HFT 常用的 ABA 解决方案？

---

## 参考与延伸

- 下一节：[7.4 无锁队列（SPSC）](04-spsc-queue.md)
- 上一节：[7.2 无锁栈（Treiber Stack）](02-treiber-stack.md)
- 回到：[第 7 章](README.md)
