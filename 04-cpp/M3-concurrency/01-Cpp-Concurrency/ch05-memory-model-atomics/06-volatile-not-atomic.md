# 5.6 volatile ≠ atomic

> 第 5 章 · 上一节：[5.5 原子标志同步](05-atomic-flag.md) · 下一章：[第 6 章 基于锁的容器](../ch06-lock-based-containers/README.md)

## 这节讲什么

`volatile` 只防编译器优化，**不保证**原子性、可见性、内存序。多线程共享变量必须用 `std::atomic`。这是 C 转 C++ 程序员最容易犯的错误之一。

## 为什么要学这个（先建立直觉）

很多 C 嵌入式程序员"习惯"用 `volatile` 做线程间共享：

```c
// C 嵌入式代码中常见的错误用法
volatile int flag = 0;
volatile int data = 0;

// 线程 1（中断处理/另一个线程）
data = 42;
flag = 1;

// 线程 2
while (flag != 1) {}
printf("%d\n", data);  // 期望 42，但可能不是
```

**问题**：`volatile` 只告诉编译器"别把这个变量缓存到寄存器"——每次都从内存读。但它**不保证**：

1. **原子性**：`volatile int64_t` 在 32 位机器上可能被撕裂（高 32 位和低 32 位分两次写）
2. **可见性**：一个 CPU 核写入 `volatile` 变量后，其他核的 cache 可能不会立即更新
3. **内存序**：`volatile` 不阻止 CPU/编译器重排 `data=42` 和 `flag=1`

```cpp
// C++ 正确写法
std::atomic<int> flag{0};
int data = 0;

data = 42;
flag.store(1, std::memory_order_release);  // 原子 + 可见 + 有序

while (flag.load(std::memory_order_acquire) != 1) {}
std::cout << data;  // 保证 42
```

## 本质区别详解

### volatile 做了什么

```cpp
// volatile 的作用：防止编译器优化
volatile int* hw_reg = (volatile int*)0xFFFF0000;

// 没有 volatile：编译器可能优化掉"无用"的读写
*hw_reg = 1;  // 写硬件寄存器
*hw_reg = 1;  // 编译器可能删掉这行（认为重复写无用）

// 有 volatile：编译器每次都执行读写
*hw_reg = 1;  // 执行
*hw_reg = 1;  // 也执行（volatile 保证不优化）
```

### volatile 不做什么

```cpp
volatile bool ready = false;
volatile int data = 0;

// 线程 1
data = 42;    // volatile 不阻止这条被重排到下面
ready = true; // volatile 不保证这条对其他核立即可见

// 线程 2
while (!ready) {}  // 可能永远循环（ready 不可见）
                   // 即使退出循环，data 可能不是 42
```

| 保证 | volatile | std::atomic |
|------|----------|-------------|
| 不缓存到寄存器 | ✅ | ✅ |
| 原子性 | ❌ | ✅ |
| 跨核可见性 | ❌ | ✅ |
| 内存序/阻止重排 | ❌ | ✅ |
| 适用场景 | 硬件寄存器/MMIO | 多线程共享变量 |

### 为什么 C 嵌入式代码"看起来"能用 volatile

```c
// 很多嵌入式代码在单核 + 简单编译器上"碰巧"工作
volatile int flag = 0;

// 中断处理
void ISR() {
    flag = 1;  // volatile 防止编译器缓存 → 单核上立即可见
}

// 主循环
while (!flag) {}  // 单核 + 无优化 → "碰巧"工作
```

**问题**：
1. **多核**：volatile 不保证跨核可见性——核 1 写 flag 后，核 2 的 cache 可能不更新
2. **现代编译器**：volatile 不阻止指令重排——`data=42; flag=1;` 可能被重排
3. **64 位变量**：`volatile int64_t` 在 32 位机上不是原子的——可能读到半新半旧的值

### volatile 的正确用途

```cpp
// 1. 内存映射 I/O（MMIO）
volatile uint32_t* uart_tx = (volatile uint32_t*)0x40004000;
*uart_tx = 'A';  // 写 UART 发送寄存器

// 2. 硬件寄存器
volatile uint32_t* timer = (volatile uint32_t*)0x40001000;
uint32_t t = *timer;  // 读计时器——每次都读硬件

// 3. 信号处理中的 sig_atomic_t
volatile sig_atomic_t signal_flag = 0;
void handler(int) { signal_flag = 1; }  // 信号处理函数中赋值
// sig_atomic_t 保证"足够原子"——但只在单线程 + 信号场景

// 4. setjmp/longjmp 相关的局部变量
volatile int err = 0;
if (setjmp(buf) == 0) {
    err = compute();  // volatile 防止被优化掉
} else {
    // longjmp 回来后 err 的值是有效的
}
```

## 常见错误（新手踩坑）

### 错误 1：用 volatile 做线程同步

```cpp
// 错误：volatile 不能用于线程同步
volatile bool stop = false;

// 线程 1
while (!stop) { work(); }  // 可能永远循环！

// 线程 2
stop = true;  // 线程 1 可能永远看不到
```

**修复**：用 `std::atomic<bool> stop{false}`。

### 错误 2：volatile + 64 位变量

```cpp
// 错误：32 位机上 volatile int64_t 不是原子的
volatile int64_t timestamp = 0;

// 线程 1
timestamp = 0x0000000100000002LL;  // 分两次写：高 32 位 + 低 32 位

// 线程 2
int64_t ts = timestamp;  // 可能读到 0x0000000000000002 或 0x0000000100000000
// 撕裂读！
```

**修复**：用 `std::atomic<int64_t>`（保证原子 64 位读写）。

### 错误 3：volatile 指针以为跨线程安全

```cpp
// 错误：volatile 指针不保证线程安全
volatile Object* obj;

// 线程 1
obj = new Object();  // volatile 不保证其他线程看到完整构造的对象

// 线程 2
if (obj) obj->method();  // 可能访问未完全构造的对象
```

**修复**：用 `std::atomic<Object*>` + release/acquire。

## 和 C 的区别

| 特性 | C | C++ |
|------|---|-----|
| volatile 语义 | 硬件寄存器/信号 | 同 C（**不变**） |
| 线程同步 | 无标准（用 pthread） | `std::atomic` |
| 嵌入式误用 | 常见（volatile 做同步） | 应用 atomic 纠正 |
| Java volatile | 等同 atomic（有内存屏障） | C/C++ volatile **不等同** |

**关键区别**：Java 的 `volatile` 有内存屏障语义（等同于 C++ 的 `atomic` + `seq_cst`）。C/C++ 的 `volatile` **没有**——这是从 Java 转 C++ 时最容易犯的错误。

## HFT 关联

- **`atomic` 替代 `volatile`**：行情计数器、序号用 `std::atomic<uint64_t>` 保证跨核可见性 + 原子性。
- **`volatile` 只用于 mmap**：DPDK PMD 配置寄存器映射用 `volatile`，其他场景一律 `atomic`。
- **代码审查**：HFT 代码审查中 `volatile` 在非 MMIO 场景出现 = 红旗——必须改成 `atomic`。

## 代码自测

### Q1: 下列代码可能发生什么？

```cpp
volatile bool ready = false;
int data = 0;

// 线程 1
data = 42;
ready = true;

// 线程 2
while (!ready) {}
std::cout << data;
```

<details>
<summary>答案与复习指引</summary>

**多种问题可能发生**：
1. **永远循环**：编译器可能把 `ready` 缓存到寄存器——`while(!ready)` 变成死循环。volatile 应该防止这个问题，但某些编译器优化可能仍有问题。
2. **data 不是 42**：volatile 不阻止 `data=42` 和 `ready=true` 被重排。线程 2 可能看到 `ready=true` 但 `data` 还是旧值。
3. **跨核不可见**：多核 CPU 上，线程 1 写 `ready` 后，线程 2 的 cache 可能不立即更新。

修复：`std::atomic<bool> ready` + `release`/`acquire`。

复习：volatile 只防编译器寄存器缓存，不保证原子性/可见性/内存序。
</details>

### Q2: 下列代码在 32 位机器上可能出什么问题？

```cpp
volatile int64_t counter = 0;

// 线程 1
counter = 0x00000001FFFFFFFFLL;

// 线程 2
int64_t val = counter;
std::cout << val;
```

<details>
<summary>答案与复习指引</summary>

**撕裂读**。32 位机器上 `int64_t` 的读写分两条指令（高 32 位 + 低 32 位）。volatile 不保证这两条指令的原子性——线程 2 可能读到 `0x00000000000FFFFFFF`（低 32 位已更新但高 32 位未更新）或 `0x00000001FFFFFFFF`（完整）或其他组合。

修复：`std::atomic<int64_t>`——保证 64 位读写的原子性（在 x86 上用 `cmpxchg8b` 或 `movq`）。

复习：volatile 不保证多字节变量的原子性。大于机器字长的变量必须用 atomic。
</details>

### Q3: 下列哪个用法正确？

```cpp
// A: volatile 做线程标志
volatile bool stop = false;

// B: volatile 读硬件寄存器
volatile uint32_t* reg = (volatile uint32_t*)0xFFFF0000;
uint32_t val = *reg;

// C: volatile 保护共享计数器
volatile int counter = 0;
counter++;
```

<details>
<summary>答案与复习指引</summary>

**只有 B 正确**。

- **A 错误**：volatile 不保证跨核可见性/内存序——用 `std::atomic<bool>`。
- **B 正确**：volatile 的正确用途——内存映射 I/O，防止编译器优化掉硬件寄存器读写。
- **C 错误**：`counter++` 是读-改-写三步，volatile 不保证原子性——用 `std::atomic<int>` + `fetch_add`。

复习：volatile 用于 MMIO/硬件寄存器/信号处理，atomic 用于多线程共享变量。
</details>

### Q4: 为什么 Java 的 volatile 能用于线程同步，C++ 的不能？

<details>
<summary>答案与复习指引</summary>

**语义不同**：

| 语言 | volatile 语义 |
|------|---------------|
| Java | 等同 C++ 的 `atomic` + `seq_cst`——有内存屏障，保证可见性 + 有序性 |
| C/C++ | 只防编译器寄存器缓存——无内存屏障，不保证可见性/有序性 |

Java 的 volatile 在 JVM 层面插入了内存屏障（store-store + load-load 屏障），等价于 C++ 的 `std::atomic<T>` + `memory_order_seq_cst`。

C/C++ 的 volatile 没有任何内存屏障——它最初是为硬件寄存器/信号处理设计的，不是为多线程设计的。

从 Java 转 C++ 的程序员容易犯这个错误——以为 C++ 的 volatile 和 Java 一样有同步语义。

复习：C++ volatile ≠ Java volatile。C++ 多线程用 `std::atomic`。
</details>

---

## 参考与延伸

- 下一章：[第 6 章 基于锁的容器](../ch06-lock-based-containers/README.md)
- 回到：[第 5 章](README.md)
