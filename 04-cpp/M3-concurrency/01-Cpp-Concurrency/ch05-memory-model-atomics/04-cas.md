# 5.4 CAS（Compare-Exchange）—— 无锁的基石

> 第 5 章 · 上一节：[5.3 atomic 操作](03-atomic-ops.md) · 下一节：[5.5 原子标志同步](05-atomic-flag.md)

## 这节讲什么

CAS（Compare-Exchange）是无锁编程的核心原语。`weak` 可能虚假失败（适合循环），`strong` 不会。ABA 是 CAS 的经典问题。

## 为什么要学这个（先建立直觉）

C 程序员可能用 GCC 内建 CAS：

```c
// C：GCC 内建 CAS
int x = 0;
int expected = 0;
int desired = 1;
bool success = __atomic_compare_exchange_n(&x, &expected, desired,
    false,  // weak=false → strong
    __ATOMIC_ACQ_REL, __ATOMIC_ACQUIRE);
// 如果 x==expected：x=desired，返回 true
// 如果 x!=expected：expected=x，返回 false
```

C++ 标准化了 CAS：

```cpp
// C++：std::atomic CAS
std::atomic<int> x{0};
int expected = 0;
bool success = x.compare_exchange_strong(expected, 1);
// 语义同上，但更清晰

// CAS 循环（无锁更新的标准模式）
int expected = x.load();
while (!x.compare_exchange_weak(expected, desired)) {
    // 失败时 expected 被自动更新为当前值
    // 可能需要重新计算 desired
}
```

CAS 是几乎所有无锁数据结构的基础——无锁栈、无锁队列、无锁链表都用 CAS 实现。

## 核心用法详解

### compare_exchange_strong

```cpp
std::atomic<int> x{0};

// 单次 CAS：值等于 expected 才改为 desired
int expected = 0;
bool success = x.compare_exchange_strong(
    expected,     // 期望值（引用，失败时被更新）
    42,           // 新值
    std::memory_order_acq_rel,  // 成功时的内存序
    std::memory_order_acquire   // 失败时的内存序（可选）
);
// 成功：x 从 0 变为 42，返回 true
// 失败：expected 被更新为 x 的当前值，返回 false
```

### compare_exchange_weak

```cpp
std::atomic<int> x{0};

// weak 可能"虚假失败"——值实际等于 expected 却返回 false
// 适合 while 循环——虚假失败只是多重试一次
int expected = x.load(std::memory_order_relaxed);
while (!x.compare_exchange_weak(
    expected, desired,
    std::memory_order_acq_rel,
    std::memory_order_relaxed
)) {
    // 失败时 expected 已被更新为当前值
    // 如果 desired 依赖 expected，需要重新计算
    // desired = compute(expected);  // 如果需要
}
// weak 在某些平台（如 ARM LL/SC）上比 strong 更高效
```

### weak vs strong

| 特性 | weak | strong |
|------|------|--------|
| 虚假失败 | 可能 | 不会 |
| 适合 | while 循环 | 单次判断 |
| 性能 | 在 LL/SC 平台上更好 | 在 cmpxchg 平台上相同 |
| 原因 | LL/SC 可能因中断/缓存冲突失败 | strong 内部可能循环直到非虚假结果 |

```cpp
// ARM 使用 LL/SC（Load-Linked/Store-Conditional）
// LL：读取值，标记缓存行
// SC：如果缓存行未被修改，写入成功；否则失败
// LL 和 SC 之间如果有中断、其他 CPU 修改同一缓存行 → SC 失败
// weak 对应单次 LL/SC，可能因干扰失败
// strong 对应 LL/SC 循环直到非干扰失败
```

### CAS 实现无锁栈

```cpp
template<typename T>
class LockFreeStack {
    struct Node {
        T data;
        Node* next;
    };
    std::atomic<Node*> head{nullptr};

public:
    void push(const T& val) {
        Node* n = new Node{val, nullptr};
        n->next = head.load(std::memory_order_relaxed);
        while (!head.compare_exchange_weak(
            n->next, n,
            std::memory_order_release,
            std::memory_order_relaxed
        )) {
            // n->next 被自动更新为当前 head，重试
        }
    }

    bool pop(T& result) {
        Node* old_head = head.load(std::memory_order_acquire);
        while (old_head &&
               !head.compare_exchange_weak(
                   old_head, old_head->next,
                   std::memory_order_acquire,
                   std::memory_order_relaxed
               )) {
            // old_head 被自动更新，重试
        }
        if (old_head) {
            result = old_head->data;
            delete old_head;  // 注意：ABA 问题！
            return true;
        }
        return false;
    }
};
```

### ABA 问题

```
时间线：
  线程 1                    线程 2
  ────────                  ────────
  读 head = A
  准备 CAS(A → C)
                            pop A → head = B
                            pop B → head = nullptr
                            push A → head = A（A 被重新插入！）
  CAS(A → C) 成功！
  但 A 已经被弹出过了——
  它的 next 指针可能指向已释放的内存

问题：CAS 只比较值，不关心"中间发生了什么"
```

**解决方案**：
1. **Tagged Pointer**：指针 + 版本号，每次修改版本号+1
2. **Hazard Pointer**：标记"正在使用的指针"，延迟回收
3. **Epoch-based Reclamation**：周期性回收，确保没有线程在使用时才释放

## 常见错误（新手踩坑）

### 错误 1：循环里用 strong

```cpp
// 不理想：循环里用 strong——LL/SC 平台上多余重试
while (!x.compare_exchange_strong(expected, desired)) {}
// strong 内部可能已经循环了多次

// 更好：循环里用 weak
while (!x.compare_exchange_weak(expected, desired)) {}
// weak 单次 LL/SC，失败就外层循环
```

### 错误 2：CAS 后不更新 expected

```cpp
// 错误：忘记 expected 是引用——失败时会被修改
int expected = 0;
while (!x.compare_exchange_weak(expected, 42)) {
    // expected 已被更新为当前值！
    // 如果不处理 expected，下一次 CAS 用的还是旧的 expected
    // 实际上 compare_exchange 会自动更新 expected，所以这里不需要手动更新
    // 但如果 desired 依赖 expected，需要重新计算
}
```

### 错误 3：pop 后立即 delete（ABA）

```cpp
// 错误：pop 后立即 delete——其他线程可能还在用这个节点
Node* old = head.load();
if (head.compare_exchange_strong(old, old->next)) {
    delete old;  // 如果其他线程正在读 old → use-after-free！
}
// 解决：用 hazard pointer 或 epoch 回收
```

## 和 C 的区别

| 特性 | C (GCC 内建) | C++ (std::atomic) |
|------|-------------|-------------------|
| CAS | `__atomic_compare_exchange_n` | `compare_exchange_strong/weak` |
| weak/strong | 手动控制 | 标准区分 |
| 内存序 | `__ATOMIC_*` | `memory_order_*` |
| 语法 | 复杂（6 参数） | 清晰 |

## HFT 关联

- **CAS 自旋 vs mutex**：临界区极短（几条指令）且竞争不激烈时，CAS 自旋比 mutex 快（避免上下文切换）。高竞争下 CAS 自旋烧 CPU。
- **SPSC 无锁队列**：单生产者单消费者队列用序列号 + 原子读写，无需 CAS——比 MPMC 队列简单且快。
- **ABA 在 HFT 中的风险**：HFT 无锁队列如果用 CAS pop + 立即释放，ABA 会导致 use-after-free——HFT 用序列号 + 双缓冲替代 CAS。

## 代码自测

### Q1: 下列 CAS 循环有什么问题？

```cpp
std::atomic<int> x{0};
int expected = 0;
int desired = 42;

while (!x.compare_exchange_strong(expected, desired)) {
    // 空循环体
}
```

<details>
<summary>答案与复习指引</summary>

两个问题：
1. **用 strong 而非 weak**：循环中应该用 `compare_exchange_weak`——在 LL/SC 平台上更高效。
2. **如果 x 持续被其他线程修改**：这个循环会无限自旋（活锁）——需要考虑退避策略或超时。

修复：
```cpp
while (!x.compare_exchange_weak(expected, desired)) {
    // expected 已自动更新
}
```

复习：循环用 weak，单次判断用 strong。
</details>

### Q2: 下列无锁栈的 pop 有什么 bug？

```cpp
bool pop(T& result) {
    Node* old = head.load(std::memory_order_acquire);
    if (!old) return false;
    result = old->data;  // ① 读数据
    if (head.compare_exchange_strong(old, old->next,  // ② CAS
            std::memory_order_release)) {
        delete old;  // ③ 释放
        return true;
    }
    return false;  // CAS 失败，重试
}
```

<details>
<summary>答案与复习指引</summary>

**ABA 问题 + 读取后释放**：

1. **ABA**：线程 1 读 `old=A`，线程 2 pop A → pop B → push A（A 被重新插入）。线程 1 CAS(A→A->next) 成功——但 A 的 next 可能已失效。
2. **读取后释放**：`result = old->data` 在 CAS 之前——如果此时其他线程 pop 并 delete old，`old->data` 是 use-after-free。

修复：
1. 先 CAS 再读数据（CAS 成功后再读）
2. 用 hazard pointer 或 epoch 回收延迟释放
3. 或用序列号 + 双缓冲替代 CAS

复习：ABA 是无锁数据结构的经典问题——CAS 只比值不比历史。
</details>

### Q3: 在什么条件下 CAS 自旋比 mutex 快？什么时候更慢？

<details>
<summary>答案与复习指引</summary>

**CAS 更快**：
- 临界区极短（几条指令）
- 竞争不激烈（1-2 个线程偶尔冲突）
- 无上下文切换开销（mutex 竞争时 park 线程 ~1-10μs）

**CAS 更慢**：
- 高竞争（多个线程同时 CAS → 大量失败重试 → 烧 CPU）
- 临界区长（CAS 持续重试期间数据可能被多次修改）
- 在弱内存序平台（ARM）上 CAS 有屏障开销

经验法则：竞争率 < 10% 用 CAS，> 50% 用 mutex。

复习：CAS 适合低竞争短临界区，mutex 适合高竞争长临界区。
</details>

### Q4: 为什么 HFT SPSC 队列不用 CAS？

<details>
<summary>答案与复习指引</summary>

SPSC（单生产者单消费者）队列天然无竞争——只有一个写者一个读者，不需要 CAS：

```cpp
// SPSC 无锁队列
template<typename T, size_t N>
class SPSCQueue {
    T buffer[N];
    std::atomic<size_t> write_idx{0};  // 生产者写
    std::atomic<size_t> read_idx{0};   // 消费者读

    bool push(const T& val) {
        size_t w = write_idx.load(std::memory_order_relaxed);
        size_t r = read_idx.load(std::memory_order_acquire);
        if (w - r >= N) return false;  // 满
        buffer[w % N] = val;
        write_idx.store(w + 1, std::memory_order_release);
        return true;
    }

    bool pop(T& val) {
        size_t r = read_idx.load(std::memory_order_relaxed);
        size_t w = write_idx.load(std::memory_order_acquire);
        if (r == w) return false;  // 空
        val = buffer[r % N];
        read_idx.store(r + 1, std::memory_order_release);
        return true;
    }
};
```

优势：
1. 无 CAS——只有 atomic load/store（比 CAS 快 2-3 倍）
2. 无竞争——生产者只写 write_idx，消费者只写 read_idx
3. 无 ABA——不释放节点，环形缓冲区

复习：SPSC 不需要 CAS——单写者单读者天然无竞争。CAS 是 MPMC 的工具。
</details>

---

## 参考与延伸

- 下一节：[5.5 原子标志同步](05-atomic-flag.md)
- 回到：[第 5 章](README.md)
