# 3.3 接口级竞争

> 第 3 章 · 上一节：[3.2 死锁及避免](02-deadlock.md) · 下一节：[3.4 初始化保护](04-init-protection.md)

## 这节讲什么

保护要覆盖**整个逻辑操作**，而非单步。`if(!empty()) top(); pop();` 是经典错误——三步之间有竞争窗口。

## 为什么要学这个（先建立直觉）

C 程序员可能遇到过类似的"检查-使用"竞争：

```c
// C：检查后使用——中间窗口被其他线程修改
pthread_mutex_lock(&mtx);
int is_empty = (list->size == 0);
pthread_mutex_unlock(&mtx);

if (!is_empty) {
    // ← 其他线程可能在此处 pop 了最后一个元素！
    pthread_mutex_lock(&mtx);
    void* data = list->head->data;  // 如果已被 pop → 空指针解引用！
    list_pop_head(list);
    pthread_mutex_unlock(&mtx);
}
```

C++ 的 `std::stack` 也有同样的问题——每个成员函数各自加锁，但函数之间的间隙没有保护：

```cpp
// C++：标准 stack 的接口级竞争
std::stack<int> s;  // 假设内部已加锁的线程安全 stack

// 错误：三步操作之间有竞争窗口
if (!s.empty()) {       // ① 检查非空（加锁→解锁）
    auto v = s.top();   // ② 取栈顶（加锁→解锁）← 可能已被其他线程 pop！
    s.pop();             // ③ 弹出（加锁→解锁）← pop 空栈 UB
}
```

## 经典错误详解

### 问题根源

```
时间线：
线程 A                          线程 B
────────                        ────────
empty() → false (解锁)
                                pop() → 栈变空 (解锁)
top() → UB！（空栈）
```

每个成员函数内部是线程安全的，但组合操作不是。问题在于**锁的粒度**：锁保护了单个操作，但没有保护操作序列。

### 正确方案 1：持锁覆盖整个操作

```cpp
// 方案 1：暴露 mutex，让调用方持锁
class ThreadSafeStack {
    std::stack<int> data;
    mutable std::mutex m;
public:
    std::mutex& get_mutex() { return m; }

    bool empty_unlocked() const { return data.empty(); }  // 假定已持锁
    int top_unlocked() const { return data.top(); }       // 假定已持锁
    void pop_unlocked() { data.pop(); }                    // 假定已持锁
};

// 调用方持锁
ThreadSafeStack stack;
std::lock_guard<std::mutex> lk(stack.get_mutex());
if (!stack.empty_unlocked()) {
    auto v = stack.top_unlocked();
    stack.pop_unlocked();
}  // 析构解锁——整个操作原子
```

### 正确方案 2：提供原子操作接口

```cpp
// 方案 2：返回值 + 弹出合并为一个原子操作
class ThreadSafeStack {
    std::stack<int> data;
    mutable std::mutex m;
public:
    // 返回 optional：成功返回值，空栈返回 nullopt
    std::optional<int> try_pop() {
        std::lock_guard<std::mutex> lk(m);
        if (data.empty()) return std::nullopt;
        auto v = data.top();
        data.pop();
        return v;  // 整个操作在锁内，原子
    }
};

// 调用方：一行搞定，无竞争窗口
auto v = stack.try_pop();
if (v) use(*v);
```

### 正确方案 3：传引用（异常安全版）

```cpp
// 方案 3：传引用——避免拷贝异常
bool try_pop(int& result) {
    std::lock_guard<std::mutex> lk(m);
    if (data.empty()) return false;
    result = data.top();  // 如果拷贝抛异常，栈未被修改
    data.pop();
    return true;
}
```

| 方案 | 优点 | 缺点 |
|------|------|------|
| 持锁调用 | 灵活 | 暴露 mutex，调用方负担重 |
| 返回 optional | 简洁 | 需要拷贝（大对象开销） |
| 传引用 | 异常安全 | 调用方需预分配变量 |

## 常见错误（新手踩坑）

### 错误 1：top + pop 分离

```cpp
// 错误：标准 stack 的 top() 和 pop() 是分离的
auto v = stack.top();  // 拷贝栈顶元素
stack.pop();            // 删除栈顶元素
// 如果 top() 拷贝成功但 pop 前异常 → 元素丢失？不，pop 还没执行
// 但如果 top() 拷贝的是一个复杂对象且抛异常 → 栈未修改，安全
// 真正的问题：多线程下 top 和 pop 之间被其他线程 pop
```

**记住**：标准库 `std::stack` 故意把 `top` 和 `pop` 分开（为了异常安全），但这对多线程不友好——需要自己封装。

### 错误 2：以为 atomic 容器就安全

```cpp
// 错误：即使每个元素是 atomic，组合操作仍不安全
std::atomic<size_t> size{0};
std::atomic<int> front{0};

if (size.load() > 0) {  // ① 检查
    auto v = front.load();  // ② 读取
    size--;                  // ③ 计数——三步之间有窗口！
}
```

**修复**：用锁覆盖整个操作，或用无锁算法（CAS 循环）保证原子性。

### 错误 3：在锁内回调

```cpp
// 错误：持锁调用用户回调——回调可能也加锁 → 死锁
void for_each(std::function<void(int)> cb) {
    std::lock_guard<std::mutex> lk(m);
    for (auto& v : data) cb(v);  // cb 可能 lock(m) → 死锁
}
```

**修复**：在锁外调用回调（先拷贝数据再解锁再回调），或限制回调行为。

## 和 C 的区别

| 特性 | C | C++ |
|------|---|-----|
| 检查-使用竞争 | 手动管理锁覆盖 | 同样需要手动管理 |
| 原子操作接口 | 自己封装 | 自己封装（或用并发容器库） |
| 异常安全 | 不适用 | top/pop 分离的原因 |
| optional 返回 | 手动指针/错误码 | `std::optional` |

## HFT 关联

- **订单簿操作**：检查最优价 + 取单必须原子，否则多策略线程竞争时可能取到不一致数据。
- **无锁队列的"检查-取"**：HFT 用 SPSC 无锁队列时，生产者写后更新序列号，消费者检查序列号后读——序列号 + 数据是原子的（release/acquire 配对）。
- **快照一致性**：HFT 行情快照要求多个字段一致（如买一价 + 买一量），不能读到买一价更新后但买一量未更新的中间状态——需要 `seq_cst` 或 `release/acquire` 保证。

## 代码自测

### Q1: 下列代码有什么问题？

```cpp
class SafeQueue {
    std::queue<int> q;
    std::mutex m;
public:
    bool empty() { std::lock_guard<std::mutex> lk(m); return q.empty(); }
    int front() { std::lock_guard<std::mutex> lk(m); return q.front(); }
    void pop() { std::lock_guard<std::mutex> lk(m); q.pop(); }
};

SafeQueue sq;
if (!sq.empty()) {
    auto v = sq.front();
    sq.pop();
}
```

<details>
<summary>答案与复习指引</summary>

**接口级竞争**。`empty()` 和 `front()` 之间可能被其他线程 `pop()` 清空队列。`front()` 读空队列是 UB。

修复：提供原子操作 `try_pop()`：
```cpp
std::optional<int> try_pop() {
    std::lock_guard<std::mutex> lk(m);
    if (q.empty()) return std::nullopt;
    auto v = q.front();
    q.pop();
    return v;
}
```

复习：每个函数各自加锁不等于组合操作安全——锁必须覆盖整个逻辑操作。
</details>

### Q2: 下列代码安全吗？

```cpp
std::atomic<int> flag{0};
int data = 0;

// 线程 1
data = 42;
flag.store(1, std::memory_order_release);

// 线程 2
while (flag.load(std::memory_order_acquire) != 1) {}
std::cout << data;
```

<details>
<summary>答案与复习指引</summary>

**安全**。`release` store + `acquire` load 配对建立了 happens-before 关系：线程 1 在 `release` 前的所有写操作（`data = 42`）对线程 2 可见。线程 2 读到 `flag == 1` 后，`data` 一定是 42。

这是无锁版的"检查-使用"原子操作——用内存序代替锁保证一致性。

复习：`release`/`acquire` 配对是无锁编程的核心——替代锁来保证"检查后使用"的原子性。
</details>

### Q3: 为什么标准库 `std::stack` 把 `top()` 和 `pop()` 分开？

<details>
<summary>答案与复习指引</summary>

**异常安全**。如果 `pop()` 同时返回值，拷贝构造可能抛异常——此时元素已从栈中移除但拷贝失败，元素丢失。

分开后：`top()` 先拷贝（如果抛异常，栈未修改），成功后再 `pop()`（不抛异常）。元素不会丢失。

代价：多线程下 `top + pop` 不原子——需要自己封装。

复习：异常安全 vs 线程安全的权衡——标准库选了异常安全，多线程安全由用户负责。
</details>

### Q4: HFT 订单簿的"检查最优价 + 取单"如何保证原子？

<details>
<summary>答案与复习指引</summary>

两种方案：

**方案 1（有锁）**：用 `scoped_lock` 覆盖检查和取单：
```cpp
std::scoped_lock lk(mtx);
if (!orderbook.empty()) {
    auto order = orderbook.best();
    orderbook.remove(order.id);
}
```

**方案 2（无锁）**：用序列号 + release/acquire：
```cpp
// 写线程（行情更新）
orderbook.update(price, quantity);
seq.store(++seq_num, std::memory_order_release);

// 读线程（策略取单）
auto s = seq.load(std::memory_order_acquire);
auto price = orderbook.get_price();
auto qty = orderbook.get_quantity();
// s 保证 price 和 qty 是一致的快照
```

HFT 热路径优先用无锁方案避免锁开销。

复习：接口级竞争的解决方案——锁覆盖整个操作（简单但有开销）或无锁 CAS/内存序（高效但复杂）。
</details>

---

## 参考与延伸

- 下一节：[3.4 初始化保护](04-init-protection.md)
- 回到：[第 3 章](README.md)
