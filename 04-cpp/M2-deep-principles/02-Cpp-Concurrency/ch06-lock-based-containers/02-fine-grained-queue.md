# 6.2 线程安全队列：细粒度锁

> 第 6 章 · 上一节：[6.1 线程安全栈](01-threadsafe-stack.md) · 下一节：[6.3 锁分段哈希表](03-lock-striping.md)

## 这节讲什么

单 mutex 的队列 push 和 pop 互斥——入队和出队无法并行。本节用**头尾各一把锁**实现细粒度锁队列，让 push 和 pop 可以同时进行，大幅提升并发吞吐。核心难点是**空队列时 tail 锁要回退到 head 锁**的协调。

---

## 核心规则（代码+表格）

### 单锁队列的瓶颈

```cpp
// 单 mutex：push 和 pop 完全互斥
template <typename T>
class naive_queue {
    std::queue<T> q;
    mutable std::mutex m;
public:
    void push(const T& v) { lock_guard lk(m); q.push(v); }
    T pop() { lock_guard lk(m); auto v = q.front(); q.pop(); return v; }
};
// 即使一个线程 push、一个线程 pop，也要争抢同一把锁
```

### 细粒度锁队列（头尾分离）

关键思路：用**链表**而非 `std::queue`，head 和 tail 各持一把锁，push 锁 tail、pop 锁 head。

```cpp
template <typename T>
class fine_grained_queue {
    struct node {
        T data;
        std::unique_ptr<node> next;
        node(T d) : data(std::move(d)) {}
    };
    std::unique_ptr<node> head;   // 头节点
    node* tail = nullptr;         // 尾指针（裸指针，因为 head 管理所有权）
    std::mutex head_mutex;
    std::mutex tail_mutex;

public:
    fine_grained_queue() {
        // 哨兵节点：永远有一个 dummy，head 和 tail 不会指向同一个真实节点
        auto dummy = std::make_unique<node>(T{});
        tail = dummy.get();
        head = std::move(dummy);
    }

    void push(T value) {
        auto new_node = std::make_unique<node>(T{});  // 先建新空节点
        node* new_tail = new_node.get();
        {
            std::lock_guard<std::mutex> lk(tail_mutex);
            tail->data = std::move(value);   // 填数据到当前尾
            tail->next = std::move(new_node); // 接上新空节点
            tail = new_tail;                  // tail 指向新空节点
        }
    }

    bool pop(T& value) {
        std::lock_guard<std::mutex> lk(head_mutex);
        if (head.get() == tail) return false;  // 空队列（head 和 tail 指向同一个哨兵）
        value = std::move(head->data);
        head = std::move(head->next);   // 旧 head 自动释放
        return true;
    }
};
```

### 哨兵节点的作用

| 问题 | 哨兵节点如何解决 |
|------|-----------------|
| push 和 pop 操作同一个节点 | 哨兵让 head 和 tail 永远不指向同一个真实数据节点 |
| push 写 tail->next、pop 读 head->next | 哨兵让这两个 next 是不同节点，无需同一把锁保护 |
| 空队列判断 | `head.get() == tail` 即为空（都指向哨兵） |

### 并发度对比

| 方案 | push 和 pop 并行？ | 竞争点 |
|------|-------------------|--------|
| 单 mutex | 否 | 唯一的一把锁 |
| 头尾双锁 | 是（空队列除外） | 空队列时 pop 需判断 tail，可能需锁 tail |
| 无锁（第 7 章） | 完全并行 | CAS 重试 |

---

## 新手要点（和 C 的区别）

- **C 里实现队列通常用数组+环形缓冲**：但环形缓冲的 head 和 tail 共享同一数组，细粒度锁不好做（除非用 SPSC 无锁）。C++ 这里用链表是为了让 head 和 tail 指向不同节点。
- **`unique_ptr<node>` 管理链表**：C 里用 `malloc`/`free` 手动管理，C++ 用 `unique_ptr` 自动释放旧 head。`head = std::move(head->next)` 自动析构旧节点。
- **裸指针 `tail` 的安全性**：tail 指向的节点由 head 的链式 `unique_ptr` 管理，所以 tail 用裸指针不会泄漏。但访问 tail 必须锁 `tail_mutex`。
- **哨兵节点是关键技巧**：没有哨兵，push 和 pop 可能在边界情况（空队列→第一个 push）竞争同一节点。C 程序员可能没想到这个——哨兵让边界情况消失。

---

## HFT 关联

- **SPSC 队列在 HFT 中更常用**：HFT 行情处理通常是单生产者（网卡线程）单消费者（策略线程），此时用无锁 SPSC 环形队列（第 7 章）比细粒度锁队列快得多。
- **细粒度锁队列适用于 MPMC**：如订单管理多个线程入队、风控线程出队，细粒度锁队列是合理的折中。
- **哨兵节点的预分配**：HFT 中哨兵和真实节点都应从 mempool 分配，避免 `make_unique` 走 `new`。push 时预建空节点也可在初始化时批量分配。

---

## 自测题

1. 为什么单 mutex 队列的 push 和 pop 无法并行？瓶颈在哪里？
2. 细粒度锁队列为什么用链表而非 `std::queue`？环形缓冲可以吗？
3. 哨兵节点（dummy node）解决了什么问题？没有它会怎样？
4. `tail` 为什么用裸指针而不是 `unique_ptr`？它的所有权由谁管理？
5. 空队列时 `head.get() == tail`，此时 pop 需要锁 tail_mutex 吗？

---

## 参考与延伸

- 下一节：[6.3 锁分段哈希表](03-lock-striping.md)
- 上一节：[6.1 线程安全栈](01-threadsafe-stack.md)
- 回到：[第 6 章](README.md)
