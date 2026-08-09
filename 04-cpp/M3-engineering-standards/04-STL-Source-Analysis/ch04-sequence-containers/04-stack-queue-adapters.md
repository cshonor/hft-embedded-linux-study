# 4.4 stack/queue 适配器

> 第 4 章 序列容器 · 第 4 节 · 上一节：[4.3 deque 源码](03-deque-implementation.md) · 下一节：[第 5 章 关联容器](../ch05-associative-containers/README.md)

## 为什么要学这个（先建立直觉）

在 C 里，栈和队列是用数组或链表手写的——没有"适配器"概念，每种底层都要重新实现 push/pop。C++ STL 的 stack/queue 是**适配器**——它们不自己管理内存，而是包装底层容器，只暴露裁剪后的接口。

```c
/* C: 栈 = 数组 + top 指针 */
int stack[100], top = -1;
void push(int v) { stack[++top] = v; }
int pop() { return stack[top--]; }
// 换成链表？全部重写
```

```cpp
// C++: stack 是适配器，包装底层容器
std::stack<int> s;  // = stack<int, deque<int>>
s.push(1); s.push(2);
s.top();  // 2
s.pop();
// 换底层？改一个模板参数
std::stack<int, std::vector<int>> sv;  // 用 vector 做底层
```

**直觉**：stack/queue 不是独立容器，是"接口裁剪器"——它们包装一个序列容器，只露 LIFO/FIFO 接口，隐藏其他操作。

## 这节讲什么

### 适配器模式

```cpp
template<typename T, typename Container = std::deque<T>>
class stack {
    Container c;  // 底层容器
public:
    void push(const T& val) { c.push_back(val); }
    void pop() { c.pop_back(); }
    T& top() { return c.back(); }
    bool empty() const { return c.empty(); }
    size_t size() const { return c.size(); }
    // 不暴露 begin()/end() → 不能遍历
    // 不暴露 operator[] → 不能随机访问
    // 不暴露 insert/erase → 不能中间插入
};
```

### 三种容器适配器

| 适配器 | 默认底层 | 接口 | 底层要求 |
|--------|---------|------|----------|
| `stack` | `deque` | push/pop/top（LIFO） | push_back/pop_back/back |
| `queue` | `deque` | push/pop/front/back（FIFO） | push_back/pop_front/front/back |
| `priority_queue` | `vector` | push/pop/top（堆序） | push_back/pop_back/front + RandomAccess |

### priority_queue 的堆算法

```cpp
template<typename T, typename Container = std::vector<T>,
         typename Compare = std::less<T>>
class priority_queue {
    Container c;
    Compare comp;
public:
    void push(const T& val) {
        c.push_back(val);
        std::push_heap(c.begin(), c.end(), comp);  // 上浮
    }
    void pop() {
        std::pop_heap(c.begin(), c.end(), comp);  // 下沉
        c.pop_back();
    }
    const T& top() const { return c.front(); }
};
// 默认 less → 大顶堆（最大元素在顶）
// 改成 greater → 小顶堆
std::priority_queue<int, std::vector<int>, std::greater<int>> min_heap;
```

### 为什么无迭代器

```cpp
std::stack<int> s;
// s.begin();  // 编译错误：stack 无 begin()
// 不能遍历栈——这破坏了 LIFO 语义
// 只能通过 push/pop/top 逐个访问
```

**设计意图**：适配器通过"隐藏接口"来保证语义正确性——stack 只能 LIFO，queue 只能 FIFO。

## 常见错误（新手踩坑）

### 错误 1：试图遍历 stack/queue

```cpp
std::stack<int> s;
// for (auto& x : s) {}  // 编译错误：无 begin/end
// 要遍历必须逐个 pop（破坏性）
while (!s.empty()) {
    std::cout << s.top();
    s.pop();
}
```

### 错误 2：priority_queue 删除特定元素

```cpp
std::priority_queue<int> pq;
pq.push(1); pq.push(2); pq.push(3);
// 想删除值为 2 的元素？
// pq 不支持随机删除！只能 pop 堆顶
// 要删特定元素必须全部 pop 到目标，再 push 回去
```

### 错误 3：queue 用 vector 做底层

```cpp
std::queue<int, std::vector<int>> q;
q.push(1);
q.pop();  // 编译错误！vector 没有 pop_front
// queue 需要 pop_front，vector 不支持
// 只能用 deque 或 list
```

## 新手要点（和 C 的区别）

| 方面 | C (手写) | C++ 适配器 |
|------|---------|-----------|
| 实现 | 每种底层重写 | 包装底层容器 |
| 换底层 | 全部重写 | 改模板参数 |
| 接口保护 | 无（可误用） | 隐藏不需要的接口 |
| 迭代器 | 手写 | 故意不提供 |

## HFT 关联

- **priority_queue 做事件调度**：定时撤单/事件队列用 `priority_queue`（默认大顶堆），但注意无随机删除
- **HFT 自建可删除堆**：标准 `priority_queue` 不支持删除任意元素，HFT 常自建带索引的堆（lazy deletion 或 index map）
- **stack/queue 用 vector + reserve**：固定大小时用 `stack<T, vector<T>>` + reserve 比 deque 更 cache 友好

## 代码自测

### Q1: 适配器本质

```cpp
std::stack<int, std::vector<int>> s;
s.push(1); s.push(2); s.push(3);
s.top();  // 返回什么？
s.pop();
s.top();  // 返回什么？
```

<details>
<summary>答案</summary>

- 第一个 `top()` 返回 **3**（最后 push 的，LIFO）
- `pop()` 删除 3
- 第二个 `top()` 返回 **2**

底层 vector 中：push 后 `[1, 2, 3]`，pop 后 `[1, 2]`。stack 的 `top()` = `vector::back()`，`pop()` = `vector::pop_back()`。

**适配器本质**：stack 只是把 `push_back`→`push`、`pop_back`→`pop`、`back`→`top` 改了个名字。
</details>

### Q2: priority_queue 堆序

```cpp
std::priority_queue<int> pq;  // 默认 less = 大顶堆
pq.push(3); pq.push(1); pq.push(4); pq.push(1); pq.push(5);
// pop 的顺序是？
```

<details>
<summary>答案</summary>

**5, 4, 3, 1, 1**（降序）。

默认 `less<T>` 比较器 → 大顶堆 → `top()` 返回最大值。每次 `pop()` 取出当前最大值。

```cpp
while (!pq.empty()) {
    std::cout << pq.top() << " ";  // 5 4 3 1 1
    pq.pop();
}
```

小顶堆用 `greater`：
```cpp
std::priority_queue<int, std::vector<int>, std::greater<int>> min_pq;
// pop 顺序：1 1 3 4 5（升序）
```

**HFT**：定时事件队列用小顶堆（最早到期的在顶）。
</details>

### Q3: 底层容器要求

```cpp
// 以下哪些能编译？
std::stack<int, std::vector<int>> s1;    // A
std::stack<int, std::list<int>> s2;      // B
std::queue<int, std::vector<int>> q1;    // C
std::queue<int, std::list<int>> q2;      // D
```

<details>
<summary>答案</summary>

- **A（stack + vector）**：✅ stack 需要 push_back/pop_back/back，vector 都有
- **B（stack + list）**：✅ list 也有 push_back/pop_back/back
- **C（queue + vector）**：❌ queue 需要 pop_front，vector 没有 pop_front
- **D（queue + list）**：✅ list 有 push_back/pop_front/front/back

**规则**：
- stack 底层需要：push_back, pop_back, back → vector/deque/list 都行
- queue 底层需要：push_back, pop_front, front, back → deque/list（vector 不行）
</details>

### Q4: 自定义可删除堆

```cpp
// 标准 priority_queue 不支持删除特定元素
// HFT 事件调度需要取消定时器 → 需要可删除的堆
```
> 如何实现可删除的优先队列？

<details>
<summary>答案</summary>

**方案 1：Lazy deletion（延迟删除）**

```cpp
struct Event {
    int id; timestamp_t when;
    bool cancelled = false;
};
std::priority_queue<Event, std::vector<Event>, EventCmp> pq;
std::unordered_map<int, Event*> index;  // id → Event*

void cancel(int id) {
    index[id]->cancelled = true;  // 标记取消，不从堆中删除
}
Event top() {
    while (!pq.empty() && pq.top().cancelled) {
        pq.pop();  // 弹出已取消的（延迟清理）
    }
    return pq.top();
}
```

**方案 2：带索引的堆**（更复杂但更高效）

维护一个 `unordered_map<id, size_t>` 记录每个元素在堆数组中的位置。删除时交换到末尾、pop_back、然后下沉/上浮恢复堆序。

**HFT**：方案 1 更简单，适合取消率不高的场景。方案 2 适合频繁取消的场景（如大量定时器被取消）。
</details>

## 参考与延伸

- 上一节：[4.3 deque 源码](03-deque-implementation.md)
- 下一节：[第 5 章 关联容器](../ch05-associative-containers/README.md)
