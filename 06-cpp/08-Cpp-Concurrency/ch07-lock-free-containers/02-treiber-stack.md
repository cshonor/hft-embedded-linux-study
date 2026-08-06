# 7.2 无锁栈（Treiber Stack）

> 第 7 章 · 上一节：[7.1 无锁 vs 有锁](01-lock-free-vs-locked.md) · 下一节：[7.3 ABA 问题](03-aba.md)

## 这节讲什么

Treiber Stack 是最经典的无锁数据结构——用 CAS 实现 push 和 pop，无需 mutex。本节讲它的实现、内存序选择、以及为什么它是最简洁的无锁结构入门。

---

## 核心规则（代码+表格）

### Treiber Stack 实现

```cpp
template <typename T>
class treiber_stack {
    struct node {
        T data;
        node* next;
        node(const T& d) : data(d), next(nullptr) {}
    };
    std::atomic<node*> head{nullptr};

public:
    void push(const T& value) {
        node* n = new node(value);
        n->next = head.load(std::memory_order_relaxed);
        // CAS：如果 head 还是 n->next，则把 head 改成 n
        while (!head.compare_exchange_weak(
                   n->next, n,
                   std::memory_order_release,    // 成功：release
                   std::memory_order_relaxed));  // 失败：重试，n->next 被更新
    }

    bool pop(T& result) {
        node* old_head = head.load(std::memory_order_relaxed);
        while (old_head) {
            // CAS：如果 head 还是 old_head，则改成 old_head->next
            if (head.compare_exchange_weak(
                    old_head, old_head->next,
                    std::memory_order_acquire,    // 成功：acquire
                    std::memory_order_relaxed)) {
                result = old_head->data;
                delete old_head;   // ⚠️ 危险！见 7.3 ABA
                return true;
            }
        }
        return false;  // 空栈
    }
};
```

### push 的 CAS 语义

```
初始：head → A → B → null

线程1 push(N):
  N->next = head (A)
  CAS(head, A, N)   // head == A? yes → head = N
  
结果：head → N → A → B → null

如果线程2 同时 push(M):
  M->next = head (A)
  CAS(head, A, M)   // 线程2 先成功
  CAS(head, A, N)   // 线程1 失败，head 现在是 M，不是 A
                    // CAS 把 M 写回 n->next，重试
  N->next = M
  CAS(head, M, N)   // 成功
```

### 内存序选择

| 操作 | 成功序 | 失败序 | 理由 |
|------|--------|--------|------|
| push CAS | `release` | `relaxed` | push 后 head 指向新节点，release 保证新节点数据对 pop 方可见 |
| pop CAS | `acquire` | `relaxed` | pop 获取 head，acquire 保证读到 push 方写入的节点数据 |
| load head | `relaxed` | — | 只用于 CAS 的预期值，真正的同步靠 CAS |

### 为什么 pop 的 `delete` 有问题

```cpp
// pop 中直接 delete old_head 是危险的：
node* old_head = head.load();
// ... CAS 成功 ...
delete old_head;   // 如果另一个线程的 pop 也在读 old_head->next？
                   // → use-after-free！
```

这是无锁内存回收的核心难题，引出 7.3 ABA 和 hazard pointer / epoch reclamation。

---

## 新手要点（和 C 的区别）

- **C 里实现这个要用汇编或内建函数**：`__sync_bool_compare_and_swap`（GCC）或 `_InterlockedCompareExchange`（MSVC）。C++11 的 `atomic::compare_exchange_weak` 统一了接口，跨平台。
- **`memory_order_release/acquire` 是 C 程序员的新世界**：C 里要么不管（错误），要么全屏障（过度）。C++11 的 release/acquire 配对是"刚好够用"的精确同步。
- **`compare_exchange_weak` vs `strong`**：weak 可能伪失败（值相等也返回 false），用在循环里（因为重试代价低）；strong 保证不伪失败，用在无循环的单次 CAS。C 里没有这个区分。
- **无锁栈的 `delete` 难题是 C 程序员没遇到过的**：C 的 `free` 同样有这个问题，但 C 程序员通常不写无锁结构。C++ 无锁必须解决内存回收——这是第 7 章后半部分的主题。

---

## HFT 关联

- **Treiber Stack 在 HFT 中用于任务队列**：多生产者（策略线程）单消费者（执行线程）的任务派发，无锁栈比有锁队列快。
- **内存回收用 mempool 而非 delete**：HFT 中 Treiber Stack 的节点从 mempool 分配，pop 后归还 mempool 而非 `delete`——避免了 use-after-free 和 ABA（mempool 节点地址固定，ABA 仍存在但可配合序号解决）。
- **release/acquire 比 seq_cst 快**：x86 上 release/acquire 编译为普通 `mov`（x86 TSO 天然满足），而 seq_cst 需要 `mfence`/`lock` 前缀。HFT 热路径用 release/acquire。
- **避免在 push 里 `new`**：HFT 中 push 前的 `new node` 应改为从 mempool 取——`new` 走 malloc 有锁且不可控延迟。

---

## 自测题

1. Treiber Stack 的 push 用什么 CAS 模式？为什么需要循环？
2. push CAS 成功用 `memory_order_release`，pop CAS 成功用 `memory_order_acquire`，为什么这样配对？
3. `compare_exchange_weak` 和 `strong` 有什么区别？各自用在哪里？
4. pop 中直接 `delete old_head` 有什么危险？
5. 为什么 push 前的 `new node` 在 HFT 中应该改成 mempool 分配？

---

## 参考与延伸

- 下一节：[7.3 ABA 问题](03-aba.md)
- 上一节：[7.1 无锁 vs 有锁](01-lock-free-vs-locked.md)
- 回到：[第 7 章](README.md)
