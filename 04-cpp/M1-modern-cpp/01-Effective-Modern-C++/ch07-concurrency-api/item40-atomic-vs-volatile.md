# Item 40：std::atomic 用于并发，volatile 用于特殊内存——别混用

> 第 7 章 并发 API · Item 40 · 上一节：[Item 39 单次事件](item39-one-shot-events.md) · 下一节：[Item 41 按值传递](../ch08-tweaks/item41-pass-by-value.md)

## 为什么要学这个（先建立直觉）

这可能是 C++ 里被误解最深的两个关键字。很多从 C / 嵌入式转来的程序员有个根深蒂固的习惯：线程间共享的标志位用 `volatile`——"告诉编译器别优化，每次都从内存读，这样另一个线程的修改就能被看见"。这个习惯在单核 + 中断的年代碰巧能用，搬到多核多线程就是一颗定时炸弹。

问题的根源是名字撞车：Java 和 C# 里的 `volatile` 确实有跨线程可见性语义，而 C/C++ 的 `volatile` **和线程一点关系都没有**——它只对编译器说"这块内存可能被程序之外的东西（硬件、信号处理器）改写，不许优化我的读写"。它不管原子性，不管指令重排，不管 CPU 缓存一致性。

C++ 里表达"别的线程也会碰这个变量"的正确工具是 `std::atomic`：原子性、内存序、跨核可见性，一应俱全。本节用两组硬实测把这件事钉死：双线程各加 100 万次，`volatile int` 会丢将近一半更新，而 `atomic<int>` 精确等于 200 万；再用汇编对比展示 `volatile` 真正的本职——MMIO 寄存器轮询。

---

## 这节讲什么

`std::atomic` 与 `volatile` 是两个**正交**的工具：atomic 面向并发（管别的线程），volatile 面向特殊内存（管编译器优化）。用数据竞争实测证明 volatile 做不了并发，用汇编实测证明 volatile 拦得住编译器优化——以及为什么反过来用 atomic 访问硬件寄存器也不合适。

---

## 本质区别：一张表

| | `std::atomic` | `volatile` |
|---|---------------|------------|
| 服务对象 | 多线程并发 | 程序之外的改写者（硬件 / 信号） |
| 原子性 | 保证 | **不保证** |
| 跨线程可见性 / 内存序 | 保证（memory order） | **不保证** |
| 阻止编译器优化读写 | 顺带做到 | **唯一本职** |
| 典型场景 | 共享计数器、无锁队列、停止标志 | MMIO 寄存器、`sig_atomic_t` 标志 |

关键认知：**它们解决的是两个不同维度的问题，谁也不含谁。** 一个变量完全可以既 `atomic` 又不需要 `volatile`（普通共享变量），也可以只需要 `volatile` 而不需要原子性（单线程轮询硬件寄存器），极少数场景两者都要。

---

## 实测 1：volatile 做并发的下场（本机实测 clang 23.1.0）

两个线程各对同一个计数器自增 100 万次，分别用 `volatile int` 和 `std::atomic<int>`：

```cpp
#include <atomic>
#include <cstdio>
#include <thread>

constexpr int N = 1'000'000;

volatile int v_counter = 0;          // 数据竞争：volatile 不提供原子性
std::atomic<int> a_counter{0};

void inc_volatile() {
    for (int i = 0; i < N; ++i) ++v_counter;   // 读-改-写三步，多线程交错
}

void inc_atomic() {
    for (int i = 0; i < N; ++i)
        a_counter.fetch_add(1, std::memory_order_relaxed);
}

int main() {
    for (int round = 1; round <= 3; ++round) {
        v_counter = 0;
        a_counter = 0;
        std::thread t1(inc_volatile), t2(inc_volatile);
        t1.join(); t2.join();
        std::printf("round %d: volatile = %d (期望 2000000)\n", round, v_counter);

        std::thread t3(inc_atomic), t4(inc_atomic);
        t3.join(); t4.join();
        std::printf("round %d: atomic   = %d (期望 2000000)\n", round, a_counter.load());
    }
    return 0;
}
```

clang 23.1.0 + `-std=c++17 -Wall -O2` 实测输出：

```
round 1: volatile = 1013244 (期望 2000000)
round 1: atomic   = 2000000 (期望 2000000)
round 2: volatile = 1000000 (期望 2000000)
round 2: atomic   = 2000000 (期望 2000000)
round 3: volatile = 1008677 (期望 2000000)
round 3: atomic   = 2000000 (期望 2000000)
```

volatile 组三轮分别丢了 98.7 万、100 万、99.1 万次更新——`++` 是"读-改-写"三步，两个线程交错执行时互相覆盖，`volatile` 对此完全无能为力（它只保证每次读写真的落到内存，不保证这三步不被打断）。atomic 组三轮全部精确 200 万。注意这还是在**内存模型较严格的 arm64** 上的结果；而且按 C++ 标准，对非原子变量的并发读写是 data race，属于未定义行为——"结果偏小"只是它碰巧的一种表现，编译器理论上可以做任何事情。

---

## 实测 2：volatile 的本职——阻止编译器优化（汇编对比）

模拟 MMIO 轮询：循环读一个"会被硬件改写"的地址直到非零。对比加不加 `volatile`：

```cpp
int reg_storage = 0;   // 模拟寄存器映射的地址

int poll_plain(int* reg) {
    while (*reg == 0) { }   // 编译器认为 *reg 不会变 → 提升出循环
    return *reg;
}

int poll_volatile(volatile int* reg) {
    while (*reg == 0) { }   // 每次迭代都真实读内存
    return *reg;
}
```

clang 23.1.0 + `-O2 -S` 生成的汇编（本机实测）：

```asm
_poll_plain:                        ; 不带 volatile
    ldr     w0, [x0]                ; 只读了一次……
    ret                             ; ……然后直接返回！循环被整个优化掉

_poll_volatile:                     ; 带 volatile
LBB1_1:                             ; 真正的循环
    ldr     w8, [x0]                ; 每次迭代都重新从内存加载
    cbz     w8, LBB1_1              ; 为 0 就继续转
    ldr     w0, [x0]
    ret
```

对比触目惊心：`poll_plain` 的循环被编译器**整个删除**——只读一次、直接返回（等价于 `if (*reg==0) 死循环; else return *reg;` 被进一步简化成"反正不会变，读了直接返回"）。这就是不带 `volatile` 轮询硬件寄存器的下场：硬件把寄存器改了一万次，你的代码还停在第一次读到的值。加上 `volatile` 后，每次迭代都有真实的 `ldr`——这才是 MMIO 轮询该有的样子。

所以 `volatile` 的语义精确地说只有一条：**"每次访问都必须真正读/写内存，编译器不许缓存、不许提升、不许删除。"** 它是说给编译器听的，不是说给 CPU 或其他线程听的。

---

## 两个方向的错配

**用 volatile 做并发（实测 1 已证伪）：** 不保证原子性（`++` 丢更新）、不插入内存屏障（不阻止 CPU/编译器对周边指令重排）、不建立 happens-before 关系。多线程共享变量请用 `std::atomic` 或互斥锁。

**用 atomic 访问硬件寄存器（同样错）：** `std::atomic` 的语义对象是"内存里的变量"，标准不保证它落在某个具体地址上的单次读写在总线层面是什么样子；某些平台上 `atomic` 操作可能带锁（`is_lock_free()` 为假），MMIO 场景下既无必要也不合适。寄存器映射就用 `volatile`，它精确表达"逐字逐句照我说的读写"。

嵌入式里的真实代码经常两者都要：一个被 DMA 改写、又被多个线程读的缓冲区描述符，地址本身是 `volatile` 的，并发计数用 `atomic`——**正交需求，各用各的**。

---

## 完整可运行例子

"实测 1"即完整可运行程序（编译：`clang++ -std=c++17 -Wall -O2`，本机实测 clang 23.1.0，无需 `-pthread`）；"实测 2"用 `-O2 -S` 生成汇编即可复现对比。

---

## 常见错误（新手踩坑）

**错误 1：用 volatile 做线程停止标志**

```cpp
volatile bool stop = false;
// 线程 A: stop = true;
// 线程 B: while (!stop) { work(); }   // 无同步保证，可能永远看不到
```

**修正：** `std::atomic<bool> stop{false};`——保证跨核可见性和顺序。

**错误 2：以为 volatile 能保证原子性**

```cpp
volatile int counter = 0;
// 多线程: ++counter;   // 读-改-写三步交错 → 丢更新（实测丢一半）
```

**修正：** `std::atomic<int> counter{0}; counter.fetch_add(1);`

**错误 3：从 Java/C# 带来的直觉——以为 volatile 有跨线程语义**

**修正：** C++ 的 `volatile` 只管编译器优化。语言间同名关键字语义完全不同，这是面试和代码评审的高频坑。

**错误 4：用 atomic 访问 MMIO 寄存器**

**修正：** 硬件寄存器用 `volatile`；`atomic` 可能不 lock-free，且语义上不针对设备内存。

---

## 新手要点（和 C 的区别）

| 维度 | C 怎么做 | C++ 怎么做 | 为什么 |
|------|---------|-----------|--------|
| 线程同步 | 误用 `volatile`（单核时代碰巧能用） | `std::atomic` | C++11 标准内存模型 |
| 硬件寄存器 | `volatile` | `volatile`（完全相同） | 防编译器优化 |
| 原子操作 | 平台内建（`__sync_*`） | `std::atomic` / `fetch_add` / CAS | 跨平台可移植 |
| 内存序 | 无概念 | `memory_order_relaxed/acquire/release/...` | 精细控制可见性 |

**一句话总结：** 多线程共享变量 → `std::atomic`；硬件寄存器 / 信号标志 → `volatile`。两者是正交工具，谁也不是谁的子集。

---

## HFT 关联

- **竞争计数器**：行情包计数、订单序列号用 `std::atomic<uint64_t>`（relaxed 足够时别上 seq_cst，省屏障开销）——volatile 版本在实测里直接丢一半更新。
- **停止标志**：`std::atomic<bool> running{true}` 替代 `volatile bool`，保证所有核及时看到停机指令。
- **DPDK / 网卡寄存器**：mmap 的 PMD 配置与状态寄存器轮询用 `volatile`（或 DPDK 自己的 `rte_read32` 封装），不用 `atomic`。
- **无锁队列**：SPSC/MPMC 队列的 head/tail 指针用 `atomic` + 恰当的 memory order；`volatile` 在这里连编译器屏障都算不上（它管不了 CPU 重排）。
- **混合场景**：FPGA/网卡 DMA 写的描述符环——描述符内存 `volatile`，消费者索引 `atomic`，各管各的。

---

## 自测题

**1. `std::atomic` 和 `volatile` 的本质区别是什么？**

<details><summary>答案</summary>
atomic 面向并发：保证读写的原子性，通过 memory order 提供跨线程可见性与顺序保证。volatile 面向编译器优化：保证每次访问真实落到内存，不被缓存/提升/删除。一个管"别的线程"，一个管"程序之外的改写者"，正交。
</details>

**2. 实测中双线程对 `volatile int` 各加 100 万次的结果是多少？为什么？**

<details><summary>答案</summary>
约 101 万（三轮：1013244 / 1000000 / 1008677），丢了近一半更新。因为 `++` 是读-改-写三步，volatile 不提供原子性，两个线程交错时互相覆盖；且按标准这是 data race，本就是未定义行为。同测试的 atomic 组精确等于 2000000。
</details>

**3. `volatile` 做线程同步缺哪三样东西？**

<details><summary>答案</summary>
①原子性——复合操作可被打断；②内存序——不插屏障，不阻止 CPU/编译器对周边访问重排；③happens-before——一个线程的写对另一个线程何时可见没有标准保证。
</details>

**4. 汇编实测里 `poll_plain` 和 `poll_volatile` 的区别说明了什么？**

<details><summary>答案</summary>
-O2 下 `poll_plain` 的循环被整个优化成一次 `ldr` + `ret`（编译器认定 *reg 不会变）；`poll_volatile` 保留了每次迭代真实加载的循环。说明 volatile 的唯一本职就是阻止编译器优化内存访问——这正是 MMIO 轮询需要的，也正是它对并发无能为力的原因。
</details>

**5. 为什么不该用 `std::atomic` 访问硬件寄存器？**

<details><summary>答案</summary>
atomic 的语义对象是"内存变量"，不针对设备内存；某些平台上 atomic 操作可能带锁（非 lock-free），对 MMIO 既无必要也可能出错。寄存器读写用 volatile 精确表达"逐次真实访问"。
</details>

**6. 为什么单核嵌入式时代的 `volatile` 同步"碰巧能用"，多核时代就崩了？**

<details><summary>答案</summary>
单核 + 中断场景没有真正的并行执行，唯一问题是编译器把轮询读优化掉——volatile 恰好解决这个。多核的真正难点是 CPU 乱序、store buffer、缓存一致性带来的可见性与顺序问题，volatile 对这些一概不管。
</details>

**7. 一个变量能否同时需要 `atomic` 和 `volatile`？举例。**

<details><summary>答案</summary>
能，两者正交。例：被 DMA 引擎异步改写、又被多个工作线程并发读取的描述符环——描述符内存本身要 volatile（硬件在改），消费者/生产者索引要 atomic（多线程在争）。嵌入式驱动里这种组合很常见。
</details>

---

## 参考与延伸

- 上一节：[Item 39 单次事件](item39-one-shot-events.md)
- 下一节：[Item 41 按值传递](../ch08-tweaks/item41-pass-by-value.md)
- 回到：[第 7 章 并发 API](README.md)
