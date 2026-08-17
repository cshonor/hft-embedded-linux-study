# 5.3 std::atomic 操作

> 第 5 章 · 上一节：[5.2 六种内存序](02-memory-orders.md) · 下一节：[5.4 CAS](04-cas.md)

## 这节讲什么

`std::atomic` 的基本操作：store/load/fetch_add/exchange/compare_exchange，以及 `is_lock_free` 检查。这是无锁编程的基本工具集。

## 为什么要学这个（先建立直觉）

C 程序员用 GCC 内建函数或平台汇编做原子操作：

```c
// C：GCC 内建原子操作
int x = 0;
__atomic_add_fetch(&x, 1, __ATOMIC_RELAXED);  // 原子加
int old = __atomic_fetch_add(&x, 1, __ATOMIC_SEQ_CST);  // 返回旧值
// 或用平台汇编
// asm volatile("lock addl $1, %0" : "+m"(x));

// Windows：InterlockedIncrement(&x);
```

C++11 标准化了原子操作，类型安全且跨平台：

```cpp
// C++：std::atomic
std::atomic<int> x{0};
x.fetch_add(1, std::memory_order_relaxed);  // 原子加，返回旧值
int old = x.exchange(5);  // 原子交换，返回旧值
// 不需要平台条件编译，不需要内联汇编
```

## 核心操作详解

### 基本读写

```cpp
std::atomic<int> x{0};

// store：原子写
x.store(42, std::memory_order_release);

// load：原子读
int v = x.load(std::memory_order_acquire);

// 也可以用 operator= 和隐式转换（默认 seq_cst）
x = 42;        // 等价于 store(42, seq_cst)
int v2 = x;    // 等价于 load(seq_cst)
```

### 读-改-写（RMW）操作

```cpp
std::atomic<int> x{0};

// fetch_add：原子加，返回旧值
int old1 = x.fetch_add(1);  // x: 0→1, 返回 0
// 也可以用 operator
x += 1;  // 等价于 fetch_add(1, seq_cst)
x++;     // 等价于 fetch_add(1, seq_cst)

// fetch_sub：原子减，返回旧值
int old2 = x.fetch_sub(1);  // x: 1→0, 返回 1

// fetch_or / fetch_and / fetch_xor：位操作
x.fetch_or(0xFF);  // 原子或

// exchange：原子交换，返回旧值
int old3 = x.exchange(99);  // x→99, 返回旧值
```

### CAS（Compare-Exchange）

```cpp
std::atomic<int> x{0};

// compare_exchange_strong：值等于 expected 才改为 desired
int expected = 0;
bool success = x.compare_exchange_strong(expected, 1);
// 如果 x==0：x 变为 1，返回 true
// 如果 x!=0：expected 被更新为 x 的当前值，返回 false

// compare_exchange_weak：可能虚假失败
while (!x.compare_exchange_weak(expected, desired)) {
    // 失败时 expected 已更新为当前值，自动重试
}
```

### is_lock_free

```cpp
// 检查是否真正无锁（CPU 原子指令实现）
std::atomic<int> a;
if (a.is_lock_free()) {
    // 真正无锁——用 CPU 原子指令（如 lock cmpxchg）
} else {
    // 内部用 mutex——有锁，性能差
}

// 编译期检查
static_assert(std::atomic<int>::is_always_lock_free);  // C++17
// int、指针等 ≤ 机器字长的类型通常 is_always_lock_free
```

### 操作总结

| 操作 | 语义 | 返回值 | 典型用途 |
|------|------|--------|----------|
| `store(val, order)` | 原子写 | void | 设置值 |
| `load(order)` | 原子读 | 当前值 | 读取值 |
| `exchange(val, order)` | 原子交换 | 旧值 | 替换值 |
| `fetch_add(n, order)` | 原子加 | 旧值 | 计数器 |
| `fetch_sub(n, order)` | 原子减 | 旧值 | 计数器 |
| `compare_exchange` | 条件替换 | bool | CAS 循环 |
| `is_lock_free()` | 检查实现 | bool | 性能检查 |

## 常见错误（新手踩坑）

### 错误 1：大类型 atomic 不是 lock-free

```cpp
// 错误：假设所有 atomic 都是 lock-free
struct BigStruct { int data[100]; };
std::atomic<BigStruct> a;
// a.is_lock_free() 可能是 false——内部用 mutex！
// HFT 热路径上使用 = 有锁 = 有调度抖动
```

**修复**：检查 `is_lock_free()`，或只用 ≤ 64 位的类型。C++17 用 `is_always_lock_free` 编译期检查。

### 错误 2：用 operator 形式不知道内存序

```cpp
// 隐含 seq_cst——热路径上可能有 mfence 开销
std::atomic<int> counter{0};
counter++;  // 等价于 fetch_add(1, seq_cst)
// 如果只需要 relaxed：
counter.fetch_add(1, std::memory_order_relaxed);  // 无屏障
```

### 错误 3：atomic 不保证组合操作的原子性

```cpp
// 错误：以为两个 atomic 操作组合是原子的
std::atomic<int> a{0}, b{0};
a.store(1);  // 原子
b.store(2);  // 原子
// 但 a=1 和 b=2 之间不是原子的——其他线程可能看到 a=1 但 b=0

// 如果需要组合原子性：用一个 atomic + 位域，或用 mutex
```

## 和 C 的区别

| 特性 | C (GCC 内建) | C++ (std::atomic) |
|------|-------------|-------------------|
| 语法 | `__atomic_add_fetch(&x, 1, __ATOMIC_RELAXED)` | `x.fetch_add(1, memory_order_relaxed)` |
| 类型安全 | 无（void*） | 有（模板） |
| 跨平台 | 需要条件编译 | 标准 |
| is_lock_free | 无 | 有 |
| operator 重载 | 无 | 有（`x++`, `x = 1`） |

## HFT 关联

- **行情计数器**：`std::atomic<uint64_t>` 的 `fetch_add(relaxed)` 用于吞吐量统计，无同步开销。
- **检查 lock_free**：HFT 热路径的原子变量必须 `is_lock_free()`，否则退化为 mutex 有调度抖动。
- **避免 seq_cst operator**：HFT 代码规范禁止 `x++` 形式的原子操作——必须显式写内存序，避免意外 seq_cst。

## 代码自测

### Q1: 下列代码中 `is_lock_free()` 可能返回 false 吗？

```cpp
struct Pair { int a, b; };
std::atomic<Pair> ap;
std::cout << ap.is_lock_free();
```

<details>
<summary>答案与复习指引</summary>

**可能返回 false**。`Pair` 是 8 字节（两个 int），在 64 位平台上通常 lock-free（用 `cmpxchg16b` 或 8 字节原子操作）。但在 32 位平台上可能不是 lock-free——编译器用内部 mutex 实现。

`is_lock_free()` 是运行时检查。C++17 的 `is_always_lock_free` 是编译期常量，更可靠。

修复：用 `static_assert(std::atomic<Pair>::is_always_lock_free)` 在编译期检查。

复习：大于机器字长的类型可能不是 lock-free。HFT 必须检查。
</details>

### Q2: 下列两段代码哪个更快？

```cpp
std::atomic<int> counter{0};

// A:
counter++;  // operator 形式

// B:
counter.fetch_add(1, std::memory_order_relaxed);
```

<details>
<summary>答案与复习指引</summary>

**B 更快**（在 x86 上）。`counter++` 等价于 `fetch_add(1, seq_cst)`——x86 上 `seq_cst` 的 RMW 操作需要 `lock` 前缀（`lock xadd`），有全局屏障开销。

`fetch_add(1, relaxed)` 在 x86 上也是 `lock xadd`（x86 的 RMW 自带 lock 前缀），但不需要额外的 `mfence`。

实际差异在 x86 上很小（因为 RMW 本身就有 lock 前缀），但在 ARM 上差异更大——`seq_cst` 需要额外的 `dmb ish` 屏障。

复习：`operator++` = `seq_cst` = 最慢。明确写 `relaxed` 更快。
</details>

### Q3: 下列代码原子吗？

```cpp
std::atomic<int> a{0}, b{0};

// 线程 1
a.store(1);
b.store(2);

// 线程 2
int va = a.load();
int vb = b.load();
// 可能 va=1, vb=0 吗？
```

<details>
<summary>答案与复习指引</summary>

**可能**。每个 `store`/`load` 是原子的，但 `a.store(1)` 和 `b.store(2)` 之间不是原子的。线程 2 可能在两个 store 之间读取——看到 `a=1` 但 `b=0`。

如果需要 a 和 b 的组合原子性：
1. 用一个 `atomic<uint64_t>` 打包两个 int（位域）
2. 用 mutex 保护组合操作
3. 用 `seq_cst` + release/acquire 保证顺序（但不保证组合原子性）

复习：单个 atomic 操作是原子的，多个 atomic 操作的组合不是原子的。
</details>

### Q4: 为什么 HFT 代码规范禁止 `atomic::operator++`？

<details>
<summary>答案与复习指引</summary>

三个原因：
1. **隐式 seq_cst**：`x++` 等价于 `fetch_add(1, seq_cst)`——HFT 热路径不需要 seq_cst 的全局总序保证，额外屏障浪费。
2. **可读性**：`x.fetch_add(1, memory_order_relaxed)` 明确表达意图——"这里不需要同步"。`x++` 不清楚是 atomic 还是普通 int。
3. **一致性**：规范要求所有 atomic 操作显式写内存序——防止开发者不假思索地用默认 seq_cst。

复习：HFT 规范——所有 atomic 操作必须显式写内存序，禁止 operator 形式。
</details>

---

## 参考与延伸

- 下一节：[5.4 CAS](04-cas.md)
- 回到：[第 5 章](README.md)
