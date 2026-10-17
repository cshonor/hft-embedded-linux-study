# 第 7 章 设计无锁数据结构

**Designing Lock-Free Data Structures**

## 本章讲什么

无锁（lock-free）数据结构不用 mutex，靠原子操作（CAS、fetch_add）和内存序实现线程安全。本章讲无锁栈、无锁队列的设计，ABA 问题的成因与解决，以及内存回收难题（hazard pointer / epoch reclamation）。这是底层性能优化的核心知识，但也最容易写错。

## 要点

### 无锁 vs 有锁

| 维度 | 有锁（mutex） | 无锁（lock-free） |
|------|---------------|-------------------|
| 阻塞 | 持锁线程阻塞其他 | 至少一个线程总能推进 |
| 优先级反转 | 可能（低优先级持锁阻塞高优先级） | 不发生 |
| 上下文切换 | 竞争时有 | 无 |
| 公平性 | OS 调度决定 | 无保证（可能饥饿） |
| 实现复杂度 | 低 | 高（内存序、ABA、回收） |
| 适用 | 通用 | 热路径、低竞争、短临界区 |

三个层级：
- **wait-free**：每个操作在有限步内完成（最强，最难实现）
- **lock-free**：至少一个线程能推进（CAS 自旋）
- **obstruction-free**：无其他线程干扰时能完成

### 无锁栈（Treiber Stack）

```cpp
template <typename T>
class lockfree_stack {
    struct node {
        T data;
        node* next;
        node(const T& d) : data(d) {}
    };
    std::atomic<node*> head{nullptr};
public:
    void push(const T& v) {
        node* n = new node(v);
        n->next = head.load(std::memory_order_relaxed);
        while (!head.compare_exchange_weak(n->next, n,
                   std::memory_order_release, std::memory_order_relaxed));
    }
    bool pop(T& out) {
        node* old = head.load(std::memory_order_acquire);
        while (old && !head.compare_exchange_weak(old, old->next,
                   std::memory_order_acquire, std::memory_order_relaxed));
        if (!old) return false;
        out = old->data;
        delete old;   // 危险！见 ABA 与回收问题
        return true;
    }
};
```

`push`：把新节点 next 指向当前 head，CAS head。失败说明有人抢先，更新 next 重试。
`pop`：读 head，把 head CAS 成 old->next。失败重试。

### ABA 问题

`pop` 中的致命场景：
1. 线程 A 读到 `head = X`，准备 CAS 成 `X->next = Y`
2. 线程 B pop X、pop Y、push X（X 被回收又重用）
3. 线程 A 的 CAS 比较 `head == X` 成功（值没变！），把 head 设成 Y
4. 但 Y 已经被释放了——**use-after-free**

ABA 的本质：CAS 只比较值，不比较"有没有被改过"。值回到原样就骗过 CAS。

**解决方案**：

| 方案 | 思路 | 代价 |
|------|------|------|
| 版本号（tagged pointer） | CAS 时同时比较指针+版本号 | 指针需打包，128 位 CAS |
| Hazard Pointer | 每线程登记"我正在用的指针"，回收前检查 | 额外原子写，延迟回收 |
| Epoch Reclamation | 全局代际计数，确认所有线程离开旧代后回收 | 需要线程定期进/出 epoch |
| RCU（Read-Copy-Update） | 延迟回收直到所有读者退出 | Linux 内核常用 |

### 无锁队列（SPSC）

单生产者单消费者（SPSC）队列是最简单也最高效的无锁结构：

```cpp
// 环形缓冲，head/tail 各由一方独占写
alignas(64) std::atomic<size_t> head{0};   // 生产者写，消费者读
alignas(64) std::atomic<size_t> tail{0};   // 消费者写，生产者读
T buf[CAP];

void push(const T& v) {
    size_t t = tail.load(std::memory_order_relaxed);
    size_t h = head.load(std::memory_order_acquire);
    if (t - h == CAP) return;   // 满
    buf[t % CAP] = v;
    tail.store(t + 1, std::memory_order_release);
}
bool pop(T& out) {
    size_t h = head.load(std::memory_order_relaxed);
    size_t t = tail.load(std::memory_order_acquire);
    if (h == t) return false;   // 空
    out = buf[h % CAP];
    head.store(h + 1, std::memory_order_release);
}
```

SPSC 无竞争、无 CAS、无 ABA——只有 head/tail 的 acquire/release 同步。这是 HFT 最常用的队列结构。

### MPMC 的复杂度

多生产者多消费者队列需要 CAS 竞争 head/tail，回到 ABA 和回退（backoff）问题。通常用索引而非指针避免 ABA（环形缓冲天然有"版本"语义：位置循环递增）。

## HFT 关联

- **SPSC 队列是 HFT 标配**：网卡线程→策略线程用 SPSC 环形队列，零竞争、零分配、cache 友好。DPDK rte_ring 就是 SPSC/MPMC 环形队列。
- **`alignas(64)` 防伪共享**：head 和 tail 在不同 cache 行，否则生产者写 head 和消费者写 tail 互相 invalidate 缓存行，性能崩塌。
- **避免 ABA 用索引不用指针**：环形队列用 `size_t` 位置索引，天然单调递增，无 ABA。
- **Hazard Pointer 太重**：HFT 热路径不用动态分配的节点，用预分配的 mempool + 索引，彻底回避回收问题。
- **wait-free vs lock-free**：策略下单要求确定性延迟，宁可慢也要不阻塞——wait-free 的环形 SPSC 比 lock-free 的 CAS 栈更适合。
- **内存序选择**：x86 上 acquire/release 几乎免费，ARM 上有 `dmb` 屏障代价——跨平台部署要实测。

## 自测题

1. lock-free 和 wait-free 的区别是什么？HFT 为什么倾向 wait-free？
2. ABA 问题在无锁栈中如何发生？为什么 CAS 检测不到？
3. 解决 ABA 的四种方案分别是什么？SPSC 队列为什么天然无 ABA？
4. SPSC 环形队列为什么比 Treiber 栈更适合 HFT 热路径？
5. 为什么 SPSC 队列的 head 和 tail 要 `alignas(64)`？不隔离会怎样？

## 代码自测

### Q1: CAS 循环
```cpp
std::atomic<int> head{0};
std::atomic<int> tail{0};

bool enqueue(Node* node, Node* buffer[], int size) {
    int t;
    do {
        t = tail.load(std::memory_order_acquire);
        if (t - head.load(std::memory_order_acquire) >= size) return false; // 满
    } while (!tail.compare_exchange_weak(t, t + 1, std::memory_order_relaxed));
    buffer[t % size] = node;
    return true;
}
```
> `compare_exchange_weak` 的返回值是什么？为什么用循环（do-while）？weak 和 strong 的区别？

<details>
<summary>答案与复习指引</summary>

- **返回值**：成功返回 `true` 并写入新值；失败返回 `false`，`t` 被更新为当前实际值。
- **为什么循环**：CAS 可能因竞争失败（其他线程先修改了 tail），失败后重读 tail 重试——直到成功或队列满。
- **weak vs strong**：`weak` 允许**虚假失败**（值实际相等但返回 false），但在循环中用 weak 更高效（某些 CPU 上少一条指令）。`strong` 不会虚假失败，适合不在循环中的单次 CAS。

**HFT**：无锁环形缓冲区是 tick 队列的核心结构，CAS 循环 + relaxed/acquire 是标准模式。

**复习：** → [CAS 与无锁队列](./README.md)
</details>

### Q2: ABA 问题
```cpp
std::atomic<Node*> top;
// 线程 A: pop
Node* old_top = top.load();
Node* next = old_top->next;
// --- 上下文切换 ---
// 线程 B: pop A, pop B, push A (A 回到栈顶但 next 变了)
// 线程 A 恢复:
if (top.compare_exchange_weak(old_top, next)) { /* 成功，但 next 已失效 */ }
```
> 什么情况下 CAS 成功但结果是错误的？怎么解决？

<details>
<summary>答案与复习指引</summary>

**ABA 问题**：线程 A 读到 `top=A, next=B`。切换后线程 B 把 A 和 B 都弹出（A→free, B→free），又把 A 重新压入（但 A->next 现在指向别的内存）。线程 A 恢复后 CAS 比较 `top==A`（相等！），成功设置 `top=next(B)`——但 B 已被释放，**use-after-free**。

**解决方案**：
1. **标签指针（tagged pointer）**：原子指针 + 计数器，每次操作递增计数器，CAS 比较指针+计数器。
2. **延迟回收（hazard pointer / epoch-based reclamation）**：确保正在使用的节点不被回收。
3. **不用无锁栈**：HFT 实践中，单生产者-单消费者环形缓冲区天然无 ABA（头尾各一个写者）。

**复习：** → [ABA 问题](./README.md)
</details>
