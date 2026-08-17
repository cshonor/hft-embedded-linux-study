# 5.1 内存模型基础

> 第 5 章 内存模型和原子操作 · 下一节：[5.2 六种内存序](02-memory-orders.md)

## 这节讲什么

C++ 内存模型定义了多线程下操作如何对彼此可见。核心概念：happens-before、synchronizes-with、sequenced-before。理解这些是写正确无锁代码的前提。

## 为什么要学这个（先建立直觉）

C 程序员可能认为"代码按写的顺序执行"——但在多线程下，这是错的：

```c
// C 程序员的直觉：先写 data，再写 flag
data = 42;
flag = 1;

// 另一个线程等 flag
while (flag != 1) {}
printf("%d\n", data);  // 期望 42
```

**问题**：编译器和 CPU 可能重排这两行——先写 `flag=1` 再写 `data=42`。另一个线程看到 `flag==1` 时，`data` 可能还是旧值。

```
线程 1 的可能执行顺序（CPU 重排后）：
  flag = 1;     // 先写 flag
  data = 42;    // 后写 data

线程 2：
  while (flag != 1) {}  // 看到 flag==1
  printf("%d", data);   // 打印旧值！
```

C++ 内存模型定义了**哪些重排允许、哪些不允许**，让你能写出正确的多线程代码：

```cpp
// C++ 正确写法：用 atomic + release/acquire 保证顺序
data = 42;
flag.store(1, std::memory_order_release);  // release 阻止 data=42 被重排到后面

while (flag.load(std::memory_order_acquire) != 1) {}  // acquire 阻止读 data 被重排到前面
std::cout << data;  // 保证读到 42
```

## 核心概念详解

### 三个基础关系

| 概念 | 含义 | 作用域 |
|------|------|--------|
| sequenced-before | 同一线程内语句的先后顺序 | 单线程 |
| synchronizes-with | 一个线程的释放操作与另一个线程的获取操作同步 | 跨线程 |
| happens-before | sequenced-before + synchronizes-with 的传递闭包 | 全局 |

### happens-before 的传递性

```
线程 1：                        线程 2：                        线程 3：
data = 42;                      while(!ready.load(acq)){}       while(!done.load(acq)){}
ready.store(1, rel);  ──sync──→ val = data;                    done.store(1, rel);  ──sync──→ print(val);
                  happens-before ─────────────→                 happens-before ────→

最终：data=42 happens-before print(val)
      → 线程 3 一定能看到 data=42 的效果
```

**关键**：happens-before 是传递的——A hb B, B hb C → A hb C。这让你能通过中间变量建立跨线程的可见性链。

### 修改顺序（Modification Order）

```cpp
std::atomic<int> x{0};

// 线程 1：x.store(1);
// 线程 2：x.store(2);
// 线程 3：x.store(3);

// 所有线程对 x 的写入有一个全局一致的总序
// 可能是 1→2→3 或 2→1→3 等，但所有线程看到的顺序一致
// 这就是"修改顺序"——每个原子变量有自己的修改顺序
```

### 建立 happens-before 的方式

```
方式 1：release-acquire 配对
  线程 A：...写数据... → release store
  线程 B：acquire load → ...读数据...
  → A 中 release 前的所有写对 B 中 acquire 后的读可见

方式 2：mutex lock/unlock
  线程 A：lock → ...写数据... → unlock
  线程 B：lock → ...读数据... → unlock
  → unlock synchronizes-with 下一次 lock
  → A 中 unlock 前的写对 B 中 lock 后的读可见

方式 3：线程创建/join
  线程 A：...写数据... → std::thread(B)  // 创建线程
  → A 中创建前的写对 B 可见

  线程 B：...写数据... → 线程结束
  线程 A：t.join()  // 等待 B
  → B 中的写对 join 后的 A 可见
```

## 常见错误（新手踩坑）

### 错误 1：以为代码顺序就是执行顺序

```cpp
// 错误：以为 data 一定先于 flag 写入
data = 42;
flag = true;
// CPU 可能重排为 flag=true → data=42
// 多线程下可能读到 flag=true 但 data 仍是旧值
```

**修复**：用 `std::atomic` + 正确的内存序阻止重排。

### 错误 2：用 relaxed 做同步

```cpp
// 错误：relaxed 不建立 happens-before
std::atomic<bool> ready{false};
int data = 0;

// 线程 1
data = 42;
ready.store(true, std::memory_order_relaxed);  // relaxed 不阻止重排！

// 线程 2
while (!ready.load(std::memory_order_relaxed)) {}
std::cout << data;  // 可能不是 42——data=42 可能被重排到 ready=true 之后
```

**修复**：用 `release`/`acquire` 配对。

### 错误 3：以为 mutex 只做互斥

```cpp
// 很多程序员以为 mutex 只是"防止同时访问"
// 实际上 mutex 还提供内存屏障：
// - unlock 时 release：之前的写对后续 lock 的线程可见
// - lock 时 acquire：之后的读看到之前 unlock 前的写
// 所以 mutex 不只是互斥，还是同步工具
```

## 和 C 的区别

| 特性 | C (C11 前) | C++ (C++11 起) |
|------|------------|----------------|
| 内存模型 | 无（依赖平台保证） | 标准定义 |
| 原子操作 | GCC `__atomic`/`__sync` | `std::atomic` |
| 内存序 | 平台相关 | 标准化 6 种 |
| happens-before | 无标准 | 标准定义 |
| 实践 | 大多用 mutex（隐式屏障） | 可选 atomic（显式屏障） |

## HFT 关联

- **理解 happens-before 是写无锁代码的前提**：HFT SPSC 无锁队列靠 release-acquire 建立 happens-before，保证消费者读到数据后才读序列号。
- **mutex 的隐式屏障**：HFT 非热路径用 mutex 时，不需要额外加内存屏障——mutex 的 lock/unlock 自带 acquire/release 语义。
- **x86 的 TSO 简化**：x86 是强内存序（TSO），大部分重排不会发生——但代码仍应写正确的内存序，保证在 ARM 上也正确。

## 代码自测

### Q1: 下列代码可能输出什么？为什么？

```cpp
int data = 0;
bool flag = false;

// 线程 1
data = 42;
flag = true;

// 线程 2
while (!flag) {}
std::cout << data;
```

<details>
<summary>答案与复习指引</summary>

**输出不确定**（可能 42 也可能 0）。`data` 和 `flag` 都是非原子变量，没有内存序保证：
1. 编译器/CPU 可能重排 `data=42` 和 `flag=true`
2. 线程 2 可能永远看不到 `flag` 变为 true（编译器可能缓存到寄存器）
3. 即使看到 `flag=true`，`data` 可能还是旧值

修复：用 `std::atomic<bool> flag` + `release`/`acquire`。

复习：没有内存模型，多线程的代码顺序和执行顺序不一定一致。
</details>

### Q2: 下列代码安全吗？

```cpp
std::mutex m;
int data = 0;

// 线程 1
{
    std::lock_guard<std::mutex> lk(m);
    data = 42;
}

// 线程 2
{
    std::lock_guard<std::mutex> lk(m);
    std::cout << data;
}
```

<details>
<summary>答案与复习指引</summary>

**安全**。mutex 的 unlock（线程 1 析构时）提供 release 语义，lock（线程 2 构造时）提供 acquire 语义。所以线程 1 中 `data=42` 对线程 2 可见。

mutex 不只是互斥——它还建立 happens-before 关系。

复习：mutex = 互斥 + 内存屏障。lock = acquire，unlock = release。
</details>

### Q3: 下列代码建立了 happens-before 吗？

```cpp
std::atomic<int> x{0};

// 线程 1
x.store(42, std::memory_order_relaxed);

// 线程 2
int v = x.load(std::memory_order_relaxed);
```

<summary>
<details>
答案与复习指引

**没有建立 happens-before**。`relaxed` 只保证原子性和修改顺序一致性，不建立 synchronizes-with 关系。线程 2 可能读到 0 或 42（取决于时序），即使读到 42，也不能保证线程 1 中 `store` 前的其他写对线程 2 可见。

`relaxed` 适合无依赖的场景（如计数器），不适合同步。

复习：只有 `release`/`acquire`（或更强的 `seq_cst`）才建立 happens-before。`relaxed` 不建立。
</details>

### Q4: 为什么 HFT 代码要写正确的内存序，即使只在 x86 上跑？

<details>
<summary>答案与复习指引</summary>

x86 是 TSO（Total Store Order）——大部分 acquire/release 操作"免费"（不需要额外屏障）。但：

1. **可移植性**：代码可能未来移植到 ARM（低延迟交易也在 ARM 上跑）。ARM 是弱内存序，acquire/release 需要显式屏障——如果代码没写正确的内存序，在 ARM 上会出 bug。
2. **正确性**：即使 x86 上"碰巧正确"，编译器重排仍可能出问题。内存序不仅约束 CPU，还约束编译器。
3. **可读性**：正确的内存序让代码意图明确——其他开发者能看出"这里需要同步"。

复习：写正确的内存序 = 防御性编程。即使当前平台不需要，也要保证代码在任何平台上正确。
</details>

---

## 参考与延伸

- 下一节：[5.2 六种内存序](02-memory-orders.md)
- 回到：[第 5 章](README.md)
