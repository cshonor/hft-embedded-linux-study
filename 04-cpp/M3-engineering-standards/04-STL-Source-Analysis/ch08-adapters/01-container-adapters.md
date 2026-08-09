# 8.1 容器适配器
> 第 8 章 适配器 · 第 1 节 · 上一节：[本章概览](README.md) · 下一节：[8.2 迭代器适配器](02-iterator-adapters.md)

## 为什么要学这个（先建立直觉）

C 里实现栈和队列要手写或用数组模拟：

```c
// C: 手写栈
int stack[100];
int top = -1;
void push(int x) { stack[++top] = x; }
int pop() { return stack[top--]; }
// 没有封装，溢出/下溢自己负责
// 队列？再写一遍，环形缓冲区
```

C++ 的 `stack`/`queue`/`priority_queue` 是**适配器**——它们不自己管理内存，而是包装底层容器，只暴露需要的接口：

```cpp
std::stack<int> s;  // 底层默认 deque
s.push(1); s.push(2);
s.top();  // 2
s.pop();

std::priority_queue<int> pq;  // 底层默认 vector + 堆算法
pq.push(3); pq.push(1); pq.push(5);
pq.top();  // 5（最大值）
```

理解适配器模式，你才能理解为什么 `stack` 没有 `begin()`/`end()`（无迭代器），以及为什么 `priority_queue` 不支持随机删除。

## 这节讲什么

容器适配器用"包装 + 接口裁剪"的方式，在已有容器上构建新的数据结构接口。

### 适配器模式

```
适配器（stack/queue/priority_queue）
    │ 包装
    └── 底层容器（deque/vector/list）
         只暴露适配器需要的操作
         隐藏不需要的操作（迭代器、随机访问等）
```

### stack：LIFO 适配器

```cpp
template<class T, class Container = std::deque<T>>
class stack {
protected:
    Container c;  // 底层容器
public:
    void push(const T& x) { c.push_back(x); }    // 尾插
    void pop()             { c.pop_back(); }       // 尾删
    T& top()               { return c.back(); }    // 访问尾
    bool empty() const     { return c.empty(); }
    size_type size() const { return c.size(); }
    // 没有 begin/end！不能遍历
};
```

`stack` 把底层容器的 `push_back`/`pop_back`/`back` 映射为 `push`/`pop`/`top`，隐藏了迭代器和随机访问。

### queue：FIFO 适配器

```cpp
template<class T, class Container = std::deque<T>>
class queue {
protected:
    Container c;
public:
    void push(const T& x) { c.push_back(x); }    // 尾插
    void pop()             { c.pop_front(); }      // 头删
    T& front()             { return c.front(); }   // 访问头
    T& back()              { return c.back(); }    // 访问尾
    // 底层容器必须有 push_back + pop_front
    // vector 没有 pop_front，所以不能做 queue 底层
};
```

`queue` 需要 `pop_front`，所以默认用 `deque`（支持双端）。`vector` 没有 `pop_front`（O(n) 移动），不能做 `queue` 底层。

### priority_queue：堆适配器

```cpp
template<class T, class Container = std::vector<T>,
         class Compare = std::less<T>>
class priority_queue {
protected:
    Container c;
    Compare comp;
public:
    void push(const T& x) {
        c.push_back(x);
        std::push_heap(c.begin(), c.end(), comp);  // 上浮
    }
    void pop() {
        std::pop_heap(c.begin(), c.end(), comp);   // 下沉
        c.pop_back();
    }
    const T& top() const { return c.front(); }     // 堆顶
    // 无迭代器！不能遍历（堆序不等于排序序）
    // 无随机删除！只能删堆顶
};
```

`priority_queue` 用 `push_heap`/`pop_heap` 维护堆结构，默认 `less` 是大顶堆（最大值在顶）。

### 三种适配器对比

| 适配器 | 默认底层 | 接口 | 语义 | 迭代器 |
|--------|---------|------|------|--------|
| `stack` | `deque` | push/pop/top | LIFO 后进先出 | 无 |
| `queue` | `deque` | push/pop/front/back | FIFO 先进先出 | 无 |
| `priority_queue` | `vector` | push/pop/top | 堆序（最大/最小在顶） | 无 |

### 可更换底层容器

```cpp
// stack 用 vector 底层
std::stack<int, std::vector<int>> sv;
sv.push(1);  // 调用 vector::push_back

// stack 用 list 底层
std::stack<int, std::list<int>> sl;

// priority_queue 用 deque 底层
std::priority_queue<int, std::deque<int>> pqd;

// priority_queue 小顶堆
std::priority_queue<int, std::vector<int>, std::greater<int>> min_heap;
```

## 常见错误（新手踩坑）

### 错误 1：对 priority_queue 用迭代器

```cpp
// ❌ priority_queue 没有迭代器
std::priority_queue<int> pq;
pq.push(1); pq.push(3); pq.push(2);
// for (auto it = pq.begin(); ...)  // 编译错误！没有 begin/end
// 只能通过 top + pop 逐个取出
while (!pq.empty()) {
    std::cout << pq.top() << " ";
    pq.pop();
}
// 输出 3 2 1（取出即销毁）
```

### 错误 2：priority_queue 删除非堆顶元素

```cpp
// ❌ priority_queue 只能删堆顶
std::priority_queue<int> pq;
pq.push(1); pq.push(3); pq.push(2);
// 想删除 2？没法直接删
// pq.erase(2);  // 没有 erase！

// 只能全部弹出直到找到目标（破坏性）
// 或自建可删除的堆
```

### 错误 3：queue 用 vector 做底层

```cpp
// ❌ vector 没有 pop_front
std::queue<int, std::vector<int>> q;  // 编译错误！
// vector::pop_front 不存在
// queue 需要 push_back + pop_front + front + back
// 只有 deque 和 list 满足
```

## 新手要点（和 C 的区别）

| C | C++ | 区别 |
|----|-----|------|
| 手写栈/队列 | `stack`/`queue` 适配器 | C++ 零手写 |
| 数组模拟，溢出自己负责 | 底层容器自动扩容 | C++ 安全 |
| 无优先队列 | `priority_queue` 堆 | C++ 内置 |
| 无接口封装 | 适配器裁剪接口 | C++ 更安全 |

## HFT 关联

- **定时器队列**：`priority_queue<Timer, vector<Timer>, greater<Timer>>` 小顶堆，最近到期在顶
- **无随机删除的限制**：HFT 事件队列需要取消定时器，但 `priority_queue` 不支持——常自建可删除堆（`vector` + lazy deletion 标记）
- **无迭代器的安全保证**：适配器无迭代器防止误遍历破坏堆序——HFT 场景这正是需要的
- **底层容器选型**：`priority_queue` 默认 `vector` cache 友好，比 `deque` 更适合堆操作（连续内存 = cache 行预取友好）

## 代码自测

### Q1: stack 为什么叫"适配器"而不是"容器"？

```cpp
std::stack<int> s;  // 底层是 deque<int>
// s.begin();  // 编译错误：没有迭代器
```
> 适配器和容器有什么本质区别？

<details>
<summary>答案与复习指引</summary>

**容器**：自己管理内存和元素存储，提供完整接口（迭代器、随机访问等）。如 `vector`/`deque`/`list`。

**适配器**：不自己管理内存，**包装**底层容器，只暴露裁剪后的接口。如 `stack`/`queue`/`priority_queue`。

**stack 隐藏了什么**：
- 迭代器（`begin`/`end`）——不能遍历
- 随机访问（`operator[]`/`at`）——不能跳转
- 中间插入/删除——只能在一端操作
- `deque` 的 `push_front` 被映射为不暴露（stack 只用 `push_back`）

**设计模式**：适配器模式——修改接口（deque 的 `push_back` → stack 的 `push`），隐藏不需要的接口。

**复习：** → [适配器模式](./01-container-adapters.md)
</details>

### Q2: priority_queue 为什么没有迭代器？

```cpp
std::priority_queue<int> pq;
pq.push(3); pq.push(1); pq.push(5);
// 想遍历输出所有元素？不能！
```
> 堆的存储顺序和排序顺序有什么关系？

<details>
<summary>答案与复习指引</summary>

**堆的存储顺序 ≠ 排序顺序**。

`priority_queue` 底层是 `vector`，元素按**堆序**存储（父 ≥ 子），不是按排序序存储。遍历 `vector` 看到的是乱序的：

```
priority_queue: push 3, 1, 5
底层 vector 可能是: [5, 1, 3]  （堆序：父5 > 子1, 子3）
排序序应该是: [1, 3, 5]
```

如果暴露迭代器，用户遍历会看到"乱序"数据，产生误解。所以 `priority_queue` 不提供迭代器，只能通过 `top` + `pop` 逐个取出（取出顺序是排序序）。

**取出全部元素**：
```cpp
while (!pq.empty()) {
    cout << pq.top() << " ";  // 5 3 1（降序）
    pq.pop();                  // 破坏性！
}
```

**HFT**：需要遍历 + 删除的场景，不用 `priority_queue`，自建 `vector` + `make_heap` + 手动管理。

**复习：** → [priority_queue 堆适配器](./01-container-adapters.md)
</details>

### Q3: 为什么 queue 不能用 vector 做底层？

```cpp
// std::queue<int, std::vector<int>> q;  // 编译错误
std::queue<int, std::deque<int>> q1;  // OK
std::queue<int, std::list<int>> q2;   // OK
```
> queue 对底层容器有什么要求？vector 为什么不满足？

<details>
<summary>答案与复习指引</summary>

**queue 的接口需求**：
- `push_back`（尾插）
- `pop_front`（头删）
- `front`（访问头）
- `back`（访问尾）
- `empty` / `size`

**vector 的问题**：`vector` 没有 `pop_front`——头部删除需要移动所有后续元素 O(n)。

**满足要求的容器**：
- `deque`：双端，`push_back`/`pop_front` 都是 O(1)
- `list`：双向链表，`push_back`/`pop_front` 都是 O(1)

**stack 的要求更宽松**（只需 `push_back`/`pop_back`/`back`），所以 `vector`/`deque`/`list` 都能做 stack 底层。

**复习：** → [queue FIFO 适配器](./01-container-adapters.md)
</details>

### Q4: priority_queue 默认是大顶堆还是小顶堆？

```cpp
std::priority_queue<int> pq;  // 默认 Compare = less<int>
pq.push(3); pq.push(1); pq.push(5); pq.push(2);

// 小顶堆
std::priority_queue<int, std::vector<int>, std::greater<int>> min_pq;
min_pq.push(3); min_pq.push(1); min_pq.push(5); min_pq.push(2);

pq.top();     // ?
min_pq.top(); // ?
```
> 两个 top 分别是什么？

<details>
<summary>答案与复习指引</summary>

- `pq.top()` = **5**（大顶堆，`less` → 最大值在顶）
- `min_pq.top()` = **1**（小顶堆，`greater` → 最小值在顶）

**记忆**：
- `less` 比较器：父 < 子 不成立 → 父 ≥ 子 → **大顶堆**
- `greater` 比较器：父 > 子 不成立 → 父 ≤ 子 → **小顶堆**

**HFT**：
- 定时器队列用小顶堆（`greater`），最近到期在顶
- 最大延迟监控用大顶堆（`less`），最大延迟在顶

**复习：** → [三种适配器对比](./01-container-adapters.md)
</details>

## 参考与延伸

- 上一节：[本章概览](README.md)
- 下一节：[8.2 迭代器适配器](02-iterator-adapters.md)
- 源码参考：`bits/stl_stack.h`、`bits/stl_queue.h`（GCC libstdc++）
