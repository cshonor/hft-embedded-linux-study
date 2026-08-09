# 4.3 deque 源码：分段连续与中控器

> 第 4 章 序列容器 · 第 3 节 · 上一节：[4.2 list 源码](02-list-implementation.md) · 下一节：[4.4 stack/queue 适配器](04-stack-queue-adapters.md)

## 为什么要学这个（先建立直觉）

在 C 里，队列通常用环形数组实现——一端入队、一端出队。但如果两端都要扩容呢？SGI deque 用"分段连续 + 中控器"实现双端动态扩容。

```c
/* C: 环形数组队列（固定大小） */
int buf[N];
int head = 0, tail = 0;
void push_front(int v) { head = (head - 1 + N) % N; buf[head] = v; }
void push_back(int v) { buf[tail] = v; tail = (tail + 1) % N; }
// 问题：大小固定，不能动态扩容
```

```cpp
// C++ deque: 分段连续 + 中控器
// 逻辑上连续，物理上分多段缓冲区
// 中控器（map）是指针数组，每段指向一块缓冲区
std::deque<int> d;
d.push_front(1);  // 头部扩容：新分配一段缓冲区
d.push_back(2);   // 尾部扩容：新分配一段缓冲区
```

**直觉**：deque 是"vector 的双端版"——两端都能 O(1) 插入/删除，但通过分段连续 + 中控器实现，代价是随机访问需要两步间接。

## 这节讲什么

### 分段连续结构

```
中控器（map）：
  [ptr0] → [缓冲区0: ____a b c d]
  [ptr1] → [缓冲区1: e f g h____]
  [ptr2] → [缓冲区2: i j k l____]
  [NULL]   (未使用)

逻辑视图：a b c d e f g h i j k l
随机访问 d[5] = map[1][1] = f  （两步间接）
```

```cpp
template<typename T, typename Alloc = std::allocator<T>>
class deque {
    T** map;          // 中控器：指针数组
    size_t map_size;  // map 的大小
    T* start_cur;     // 第一个缓冲区的当前指针
    T* finish_cur;    // 最后一个缓冲区的当前指针
    // ... map 内 start/finish 的位置
};
```

### 随机访问（两步间接）

```cpp
T& operator[](size_t n) {
    // 1. 算出在第几个缓冲区
    size_t node = (start_offset + n) / buffer_size;
    // 2. 算出在缓冲区内的偏移
    size_t offset = (start_offset + n) % buffer_size;
    return map[node][offset];
}
// 比 vector 的 *(start + n) 多一步间接（先取 map[node]，再取 [offset]）
```

### 迭代器（跨段跳转）

```cpp
template<typename T>
struct __deque_iterator {
    T* cur;       // 当前元素指针
    T* first;     // 当前缓冲区起点
    T* last;      // 当前缓冲区终点
    T** node;     // 中控器中的位置

    T& operator*() { return *cur; }

    __deque_iterator& operator++() {
        ++cur;
        if (cur == last) {
            // 到达缓冲区末尾 → 跳到下一个缓冲区
            set_node(node + 1);
            cur = first;
        }
        return *this;
    }
    // operator-- 类似，跳到前一个缓冲区

    T& operator[](size_t n) {
        // 跨段随机访问
        return *(cur + n);  // 需要处理跨段
    }
};
```

### 头尾扩容

```cpp
void push_back(const T& val) {
    if (finish_cur != finish_last) {
        // 当前缓冲区有空间
        construct(finish_cur, val);
        ++finish_cur;
    } else {
        // 当前缓冲区满 → 分配新缓冲区
        reserve_map_at_back();  // 确保 map 有空间
        *(finish.node + 1) = allocate_node();  // 新缓冲区
        construct(finish_cur, val);
        // 更新 finish 迭代器
    }
}

void push_front(const T& val) {
    if (start_cur != start_first) {
        // 当前缓冲区头部有空间
        --start_cur;
        construct(start_cur, val);
    } else {
        // 分配新缓冲区
        reserve_map_at_front();
        *(start.node - 1) = allocate_node();
        // 更新 start 迭代器
    }
}
```

### deque vs vector 对比

| 方面 | vector | deque |
|------|--------|-------|
| 内存布局 | 连续 | 分段连续 |
| 随机访问 | 1 步（`*(start+n)`） | 2 步（`map[node][offset]`） |
| 头插 | O(n) | O(1) |
| 尾插 | O(1) 均摊 | O(1) 均摊 |
| 迭代器 | 原生指针 `T*` | 自定义（跨段跳转） |
| cache | 极好 | 较好（段内连续，跨段 miss） |
| 迭代器失效 | 扩容全失效 | 头尾插不影响中间 |

## 常见错误（新手踩坑）

### 错误 1：以为 deque 随机访问和 vector 一样快

```cpp
std::deque<int> d(1000000);
std::vector<int> v(1000000);
// v[i] 一步间接，d[i] 两步间接
// d 的随机访问比 v 慢约 10-20%
```

### 错误 2：中间插入

```cpp
d.insert(d.begin() + 500000, 42);  // O(n)，需要搬移
// deque 的中间插删是 O(n)，和 vector 一样
```

### 错误 3：迭代器失效

```cpp
auto it = d.begin() + 100;
d.push_front(42);  // 中间迭代器可能失效（map 扩容）
// deque 的迭代器失效规则比 vector 复杂
```

## 新手要点（和 C 的区别）

| 方面 | C (环形数组) | C++ deque |
|------|-------------|-----------|
| 大小 | 固定 | 动态 |
| 双端扩容 | 不支持 | 支持（中控器 + 分段） |
| 随机访问 | 1 步（取模） | 2 步（map→buffer） |
| 迭代器 | 手写下标 | 自定义跨段迭代器 |

## HFT 关联

- **deque 两步间接比 vector 慢**：HFT 需要严格 O(1) 单步访问时选 vector
- **deque 的双端优势**：滑动窗口（两端进出）可用 deque，但 HFT 常用环形缓冲区替代（更轻量）
- **stack/queue 默认底层是 deque**：HFT 事件队列用 `std::queue` 时底层是 deque

## 代码自测

### Q1: 随机访问

```cpp
std::deque<int> d;
for (int i = 0; i < 1000; i++) d.push_back(i);
int x = d[500];  // 几步间接？
```

<detailf>
<summary>答案</summary>

**两步间接**：
1. `map[node]` → 取第 node 个缓冲区指针
2. `buffer[offset]` → 取缓冲区内偏移

```
d[500] = map[500 / buffer_size][500 % buffer_size]
```

如果 buffer_size = 128：`map[3][116]`

**对比 vector**：`v[500] = *(start + 500)`，一步间接。

**HFT**：热路径随机访问选 vector（一步），deque 的两步间接在百万次访问中累积延迟。
</details>

### Q2: 迭代器跨段

```cpp
std::deque<int> d;
for (int i = 0; i < 300; i++) d.push_back(i);
// 假设 buffer_size = 128
auto it = d.begin();
std::advance(it, 130);  // 跨越了一个缓冲区边界
```
> `++it` 在跨段时做什么？

<details>
<summary>答案</summary>

```cpp
__deque_iterator& operator++() {
    ++cur;
    if (cur == last) {  // 到达当前缓冲区末尾
        set_node(node + 1);  // 切换到 map 中下一个缓冲区
        cur = first;          // cur 指向新缓冲区起点
    }
    return *this;
}
```

从缓冲区 0（元素 0-127）跳到缓冲区 1（元素 128-255）时：
1. `cur` 到达 `last`（缓冲区 0 末尾）
2. `set_node(node+1)` → 切换 `node`/`first`/`last` 指向缓冲区 1
3. `cur = first` → cur 指向缓冲区 1 起点（元素 128）

**代价**：跨段时多一次 `set_node`（更新 3 个指针），以及可能的 cache miss（新缓冲区不在 cache 中）。
</details>

### Q3: deque vs vector 选型

```
需要：双端队列，头部出队、尾部入队，大小约 10000
```
> 选 deque 还是 vector + 头尾指针？

<details>
<summary>答案</summary>

**取决于访问模式**：

| 场景 | 推荐 | 原因 |
|------|------|------|
| 只需 push_back/pop_front | **vector + 环形索引** | 连续内存 + O(1)，比 deque 快 |
| 需要随机访问 | **vector + 环形索引** | 一步间接，deque 两步 |
| 需要中间插入 | **deque** | 虽然都是 O(n)，但 deque 不需要搬移全部 |
| 需要双端扩容 | **deque** | vector 不能 push_front O(1) |

**HFT 推荐方案**：固定大小环形缓冲区（vector + head/tail 索引），比 deque 更轻量、cache 更友好：

```cpp
template<typename T, size_t N>
class RingQueue {
    std::array<T, N> buf;
    size_t head = 0, tail = 0;
public:
    void push(T v) { buf[tail] = std::move(v); tail = (tail+1) % N; }
    T pop() { T v = std::move(buf[head]); head = (head+1) % N; return v; }
    T& operator[](size_t i) { return buf[(head + i) % N]; }  // 一步间接
};
```
</details>

### Q4: stack/queue 底层

```cpp
std::stack<int> s;    // 默认底层
std::queue<int> q;    // 默认底层
std::priority_queue<int> pq;  // 默认底层
```
> 三者默认底层分别是什么？

<details>
<summary>答案</summary>

- `stack` → `deque<int>`（默认），也可用 `vector`/`list`
- `queue` → `deque<int>`（默认），也可用 `list`
- `priority_queue` → `vector<int>`（默认），也可用 `deque`

```cpp
std::stack<int, std::vector<int>> s;    // 用 vector
std::queue<int, std::list<int>> q;      // 用 list
```

**为什么 stack/queue 默认用 deque 而非 vector**：
- deque 头尾 O(1) 插删（stack 需要 push_back/pop_back，queue 需要 push_back/pop_front）
- deque 扩容不搬移全部元素（只加新缓冲区），vector 扩容搬移全部

**HFT**：如果大小固定，用 `std::stack<T, std::vector<T>>` + reserve 比 deque 更 cache 友好。
</details>

## 参考与延伸

- 上一节：[4.2 list 源码](02-list-implementation.md)
- 下一节：[4.4 stack/queue 适配器](04-stack-queue-adapters.md)
