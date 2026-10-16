# 5.2 六种内存序

> 第 5 章 · 上一节：[5.1 内存模型基础](01-memory-model-basics.md) · 下一节：[5.3 atomic 操作](03-atomic-ops.md)

## 这节讲什么

六种内存序的语义与代价——从最弱的 `relaxed` 到最强的 `seq_cst`。选择正确的内存序是无锁编程的核心技能。

## 为什么要学这个（先建立直觉）

C 程序员通常不关心内存序——因为 `pthread_mutex` 隐式提供了最强的 `seq_cst` 语义。但当 C 程序员转到 C++ 无锁编程时，必须理解内存序：

```cpp
// 用 mutex 时——隐式 seq_cst，不需要操心内存序
std::mutex m;
{
    std::lock_guard<std::mutex> lk(m);
    data = 42;      // 不会被重排到 unlock 之后
}   // unlock = release
{
    std::lock_guard<std::mutex> lk(m);  // lock = acquire
    std::cout << data;  // 一定能看到 42
}

// 用 atomic 时——必须显式选择内存序
std::atomic<bool> ready{false};
int data = 0;

// 线程 1
data = 42;
ready.store(true, std::memory_order_release);  // 必须写 release

// 线程 2
while (!ready.load(std::memory_order_acquire)) {}  // 必须写 acquire
std::cout << data;  // 保证 42
// 如果写 relaxed → 不保证 42！
```

## 六种内存序详解

```cpp
enum memory_order {
    relaxed,     // 无同步，仅原子性
    consume,     // 数据依赖（实践中几乎等同 acquire，已弃用倾向）
    acquire,     // 读：之后的读写不能重排到之前
    release,     // 写：之前的读写不能重排到之后
    acq_rel,     // 读写都有：RMW 操作用
    seq_cst      // 全局总序，最强，默认
};
```

### 逐个解释

**relaxed**：只保证原子性，不保证顺序或可见性。
```cpp
std::atomic<int> counter{0};
counter.fetch_add(1, std::memory_order_relaxed);  // 只保证计数原子
// 不保证其他变量的可见性——适合无依赖的计数器
```

**acquire**（读操作）：之后的读写不能重排到此次读之前。
```cpp
// 线程 B
while (!flag.load(std::memory_order_acquire)) {}  // acquire
std::cout << data;  // data 的读不会重排到 load 之前
// 保证看到 release 前的所有写
```

**release**（写操作）：之前的读写不能重排到此次写之后。
```cpp
// 线程 A
data = 42;  // 写数据
flag.store(true, std::memory_order_release);  // release
// data=42 不会重排到 store 之后
```

**acq_rel**（读-改-写操作）：同时具有 acquire 和 release 语义。
```cpp
// RMW 操作（如 fetch_add、compare_exchange）
x.fetch_add(1, std::memory_order_acq_rel);
// acquire：之后的读写不重排到之前
// release：之前的读写不重排到之后
```

**seq_cst**（顺序一致性）：最强保证——所有线程看到所有 seq_cst 操作的全局一致顺序。
```cpp
// 默认内存序——最安全但最慢
x.store(1);  // 等价于 store(1, std::memory_order_seq_cst)
x.load();    // 等价于 load(std::memory_order_seq_cst)
// 额外保证：所有线程看到的 seq_cst 操作顺序一致
```

**consume**：数据依赖版的 acquire（实践中几乎等同 acquire，标准委员会在讨论废弃）。
```cpp
// 别用——大多数编译器把 consume 当 acquire 处理
```

### 代价对比

| 内存序 | x86 代价 | ARM 代价 | 典型用法 |
|--------|----------|----------|----------|
| `relaxed` | 0（普通 load/store） | 0 | 计数器 |
| `acquire`/`release` | ~0（TSO 几乎免费） | 1 条屏障指令 | 配对同步 |
| `seq_cst` | `mfence`/`lock` 前缀 | 2 条屏障指令 | 默认/简单场景 |

### 关键直觉

- `release` 写 + `acquire` 读配对：写线程在 release 前的所有写，对读到该值的读线程可见
- `seq_cst` 额外保证**全局总序**——所有线程看到的操作顺序一致
- `relaxed` 只保证原子变量本身的原子性，不提供跨变量同步

```
release-acquire 配对：

线程 A：                          线程 B：
  data = 42;                        while(!flag.load(acq)){}
  flag.store(1, rel);  ──sync──→    print(data);  // 保证 42

seq_cst 额外保证全局总序：

线程 A：x.store(1, seq_cst)
线程 B：y.store(2, seq_cst)
线程 C：if (x.load(seq_cst)==1 && y.load(seq_cst)==0) ...  // 不可能
线程 D：if (y.load(seq_cst)==2 && x.load(seq_cst)==0) ...  // 不可能
// seq_cst 保证：如果 C 看到 x=1，则 x 的 store 在全局序中先于 C 的 load
```

## 常见错误（新手踩坑）

### 错误 1：默认 seq_cst 在热路径上

```cpp
// 错误：HFT 热路径用默认 seq_cst
std::atomic<bool> ready{false};
ready.store(true);  // seq_cst——x86 上有 mfence 开销！
while (!ready.load()) {}  // seq_cst

// 修复：用 release/acquire
ready.store(true, std::memory_order_release);
while (!ready.load(std::memory_order_acquire)) {}
```

### 错误 2：acquire/release 不配对

```cpp
// 错误：release 写 + relaxed 读——不建立 happens-before
data = 42;
flag.store(true, std::memory_order_release);

while (!flag.load(std::memory_order_relaxed)) {}  // relaxed！
// 不能保证 data==42——relaxed 不获取 release 的同步
```

**修复**：release 必须配 acquire。

### 错误 3：用 consume（别碰）

```cpp
// 别用 consume——标准委员会在讨论废弃
// 大多数编译器把 consume 当 acquire 处理
flag.load(std::memory_order_consume);  // 别这么写
```

## 和 C 的区别

| 特性 | C (C11 前) | C++ (C++11 起) |
|------|------------|----------------|
| 内存序 | 平台相关 | 标准化 6 种 |
| 默认 | 无默认 | seq_cst |
| acquire/release | 无标准 | 标准定义 |
| 实践 | 大多用 mutex | 可选 atomic + 内存序 |

## HFT 关联

- **`seq_cst` 是热路径性能杀手**：需要 CPU 全局内存屏障（x86 上 `mfence`/`lock` 前缀），比 relaxed 慢数倍。热路径尽量用 acquire/release。
- **x86 的 TSO 优势**：x86 是 TSO（Total Store Order），acquire/load 和 release/store 几乎免费。ARM 是弱内存序，acquire/release 也有显式屏障——跨平台无锁代码要测 ARM。
- **relaxed 用于统计**：HFT 吞吐量计数器用 `fetch_add(relaxed)`，无同步开销，不影响热路径延迟。

## 代码自测

### Q1: 下列代码保证 data==42 吗？

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

**保证**。`release` store + `acquire` load 配对建立了 happens-before：
- `release` 阻止 `data=42` 被重排到 `store` 之后
- `acquire` 阻止 `cout << data` 被重排到 `load` 之前
- 线程 2 读到 `flag==1` 后，线程 1 中 `release` 前的所有写（`data=42`）对线程 2 可见

复习：release-acquire 配对 = 最常用的无锁同步模式。
</details>

### Q2: 如果把上面代码的内存序都改成 relaxed，会发生什么？

<details>
<summary>答案与复习指引</summary>

**不保证 data==42**。`relaxed` 不建立 synchronizes-with 关系：
- `data=42` 可能被重排到 `flag.store` 之后
- 线程 2 可能看到 `flag==1` 但 `data` 仍是旧值

`relaxed` 只保证 `flag` 本身的原子性，不保证其他变量的可见性。

修复：用 `release`/`acquire`。

复习：`relaxed` 适合无依赖的计数器/状态标志，不适合同步。
</details>

### Q3: 下列代码中 seq_cst 的额外保证是什么？

```cpp
std::atomic<bool> x{false}, y{false};

// 线程 1
x.store(true);  // seq_cst

// 线程 2
y.store(true);  // seq_cst

// 线程 3
bool r1 = x.load();  // seq_cst
bool r2 = y.load();  // seq_cst

// 线程 4
bool r3 = y.load();  // seq_cst
bool r4 = x.load();  // seq_cst
// r1==true, r2==false, r3==true, r4==false 可能吗？
```

<details>
<summary>答案与复习指引</summary>

**不可能**（在 seq_cst 下）。seq_cst 保证全局总序——所有 seq_cst 操作有一个所有线程一致的全局顺序。

如果 r1=true（线程 3 看到 x=true），r2=false（没看到 y=true），r3=true（线程 4 看到 y=true），r4=false（没看到 x=true）——这要求 x 的 store 在 y 的 store 之后（线程 3 的视角），同时 y 的 store 在 x 的 store 之后（线程 4 的视角）——矛盾。

用 acquire/release（非 seq_cst）则可能发生——因为 acquire/release 不保证全局总序。

复习：seq_cst = acquire/release + 全局总序。全局总序保证所有线程看到的操作顺序一致。
</details>

### Q4: 为什么 HFT 热路径用 acquire/release 而不是 seq_cst？

<details>
<summary>答案与复习指引</summary>

在 x86 上：
- `seq_cst` store 需要 `mfence` 或 `lock` 前缀——额外 ~10-30 周期
- `release` store 是普通 store（TSO 免费提供 release 语义）——0 额外开销
- `acquire` load 是普通 load（TSO 免费提供 acquire 语义）——0 额外开销

在 HFT 热路径上，每次原子操作的额外 10-30 周期累积可观。用 acquire/release 在 x86 上几乎免费，在 ARM 上也只需 1 条屏障指令（vs seq_cst 的 2 条）。

但要注意：只有不需要全局总序的场景才能用 acquire/release。如果需要"所有线程看到的顺序一致"的保证，必须用 seq_cst。

复习：x86 上 acquire/release 几乎免费，seq_cst 有 mfence 开销。HFT 热路径优先 acquire/release。
</details>

---

## 参考与延伸

- 下一节：[5.3 atomic 操作](03-atomic-ops.md)
- 回到：[第 5 章](README.md)
