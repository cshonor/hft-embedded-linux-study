# 6.1 线程安全栈：粗粒度锁入门

> 第 6 章 基于锁的并发数据结构 · 上一章：[第 5 章 内存模型与原子操作](../ch05-memory-model-atomics/06-volatile-not-atomic.md) · 下一节：[6.2 线程安全队列：细粒度锁](02-fine-grained-queue.md)

## 这节讲什么

从"用锁保护容器"升级到"设计线程安全的数据结构"。本节用**单 mutex**保护整个 `std::stack`，讲解接口级竞争、返回 `shared_ptr` 避免异常丢数据等设计要点——这是并发容器设计的入门基石。

---

## 核心规则（代码+表格）

### 问题：为什么 `top()` + `pop()` 不能拆开

```cpp
// 标准库 stack 的 top() 和 pop() 是分开的——单线程没问题
std::stack<int> s;
if (!s.empty()) {
    int v = s.top();   // ① 读栈顶
    s.pop();           // ② 弹出
}
// 多线程：①和②之间另一个线程可能 pop 走元素 → 空栈 top → UB
```

**并发栈必须把 top+pop 合并成一个原子操作**。

### 线程安全栈实现

```cpp
template <typename T>
class threadsafe_stack {
    std::stack<T> data;
    mutable std::mutex m;
public:
    void push(const T& v) {
        std::lock_guard<std::mutex> lk(m);
        data.push(v);
    }
    // 返回 shared_ptr 避免拷贝异常导致数据丢失
    std::shared_ptr<T> pop() {
        std::lock_guard<std::mutex> lk(m);
        if (data.empty()) throw std::runtime_error("empty");
        auto res = std::make_shared<T>(data.top());  // 拷贝可能在锁内抛异常
        data.pop();                                   // pop 不抛异常
        return res;
    }
    // 另一种：出参版
    bool pop(T& value) {
        std::lock_guard<std::mutex> lk(m);
        if (data.empty()) return false;
        value = data.top();   // 拷贝可能抛异常，但数据还在栈上
        data.pop();
        return true;
    }
    bool empty() const {
        std::lock_guard<std::mutex> lk(m);
        return data.empty();
    }
};
```

### 设计要点表

| 决策 | 原因 |
|------|------|
| top+pop 合并 | 避免 ①② 之间的竞争窗口 |
| 返回 `shared_ptr<T>` 而非 `T` | 若 `T` 拷贝构造抛异常，数据已弹出就丢了；`shared_ptr` 构造在锁内完成 |
| 提供 `pop(T&)` 出参版 | 让调用方选择；出参版拷贝失败时数据仍在栈上 |
| 空栈抛异常或返回 false | 不返回 `bool+T*`——让调用方明确处理空栈 |
| `empty()` 也持锁 | 读操作也必须加锁，否则和 push/pop 竞争 |

---

## 新手要点（和 C 的区别）

- **C 程序员习惯 `top()` + `pop()` 分开**：在单线程下这是好设计（如果 `T` 拷贝失败，数据还在栈上）。但多线程下必须合并——这是 C++ 并发容器和单线程 STL 的核心区别。
- **C 里没有 `shared_ptr`**：C 程序员可能想返回指针或用 `malloc`+`memcpy`，但 C++ 用 `shared_ptr` 更安全——它的构造在锁内完成，拷贝异常不会丢数据。
- **`mutable` 关键字**：`empty()` 是 `const` 方法但要锁住 `m`，所以 `m` 必须声明 `mutable`。C 里没有这个概念。

---

## HFT 关联

- **粗粒度锁是 HFT 的起点而非终点**：单 mutex 保护整个栈在低竞争下够用（如配置热更新），但热路径（行情分发）必须用细粒度锁或无锁结构。
- **`shared_ptr` 返回值的代价**：`shared_ptr` 有原子引用计数开销。HFT 里如果 `T` 是 POD 且拷贝不抛异常（如 `int64_t`、固定大小结构体），可直接返回值，避免引用计数。
- **mempool + 栈**：HFT 常用固定大小对象池 + 线程安全栈管理空闲块——`push`/`pop` 就是归还/分配，粗粒度锁在低竞争下完全够用。

---

## 自测题

1. 为什么并发栈不能把 `top()` 和 `pop()` 分开？会出现什么问题？
2. 为什么 `pop()` 返回 `shared_ptr<T>` 而不是直接返回 `T`？如果 `T` 的拷贝构造抛异常会怎样？
3. `pop(T& value)` 出参版在拷贝失败时数据是否还在栈上？为什么？
4. `empty()` 方法为什么也要加锁？它是 `const` 方法，`mutex` 成员需要什么修饰？
5. 在什么场景下粗粒度锁的线程安全栈在 HFT 中是可以接受的？

---

## 参考与延伸

- 下一节：[6.2 线程安全队列：细粒度锁](02-fine-grained-queue.md)
- 上一章：[5.6 volatile ≠ atomic](../ch05-memory-model-atomics/06-volatile-not-atomic.md)
- 回到：[第 6 章](README.md)
