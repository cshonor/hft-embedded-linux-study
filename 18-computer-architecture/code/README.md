# 03 Computer Architecture · 动手代码

与 **03 模块章节笔记** 对照；读 Hennessy 时在本目录编译运行。

每章提供 **C 和 C++ 配对**例子，对照同一概念在两种语言中的写法差异。

## Ch2 · 存储器层次设计 🔴

| 文件 | 主题 | 笔记 |
|------|------|------|
| [ch02-false-sharing.c](./ch02-false-sharing.c) | 伪共享对比: 无 padding vs cache-line padding vs per-thread | [§2.3](../chapter-02-memory-hierarchy-design/notes/section-2.3-缓存性能十项高级优化.md) |
| [ch02-false-sharing.cpp](./ch02-false-sharing.cpp) | 同上 + alignas(64) + std::atomic + lambda | 同上 · [§5.3 伪共享](../chapter-05-thread-level-parallelism/notes/section-5.3-性能分析与伪共享.md) |

## Ch5 · 线程级并行 🔴

| 文件 | 主题 | 笔记 |
|------|------|------|
| [ch05-atomics-ordering.c](./ch05-atomics-ordering.c) | memory_order 对比 + 自旋锁 + 原子计数器 | [§5.6](../chapter-05-thread-level-parallelism/notes/section-5.6-内存一致性模型.md) · [§5.5](../chapter-05-thread-level-parallelism/notes/section-5.5-同步基础.md) |
| [ch05-atomics-ordering.cpp](./ch05-atomics-ordering.cpp) | 同上 + std::atomic_flag SpinLock + lock_guard | 同上 |
| [ch05-spsc-ringbuf.c](./ch05-spsc-ringbuf.c) | 无锁 SPSC 环形队列 (release-acquire) | [§5.6](../chapter-05-thread-level-parallelism/notes/section-5.6-内存一致性模型.md) |
| [ch05-spsc-ringbuf.cpp](./ch05-spsc-ringbuf.cpp) | 同上 + 模板化 SpscQueue<T,Cap> + static_assert | 同上 |

## 编译

```bash
cd 18-computer-architecture/code

# C
gcc -Wall -Wextra -std=c11 -O2 -o ch02_false  ch02-false-sharing.c     -lpthread
gcc -Wall -Wextra -std=c11 -O2 -o ch05_ato    ch05-atomics-ordering.c   -lpthread
gcc -Wall -Wextra -std=c11 -O2 -o ch05_spsc   ch05-spsc-ringbuf.c       -lpthread

# C++
g++ -Wall -Wextra -std=c++17 -O2 -o ch02_false_cpp  ch02-false-sharing.cpp     -lpthread
g++ -Wall -Wextra -std=c++17 -O2 -o ch05_ato_cpp    ch05-atomics-ordering.cpp   -lpthread
g++ -Wall -Wextra -std=c++17 -O2 -o ch05_spsc_cpp   ch05-spsc-ringbuf.cpp       -lpthread
```

## HFT 关联

| 主题 | HFT 应用 |
|------|----------|
| 伪共享 | 订单簿统计/风控计数器必须 cache-line 隔离 |
| 内存序 | 无锁 SPSC 的 publish 序: 写 data → release store index |
| SPSC 队列 | pipeline 各 stage 间解耦: NIC RX → SPSC → 策略 → SPSC → 下单 |

## C vs C++ 对照要点

| 概念 | C | C++ |
|------|---|-----|
| 原子操作 | `_Atomic` / `stdatomic.h` | `std::atomic<T>` |
| 自旋锁 | `_Atomic int` exchange | `std::atomic_flag` test_and_set |
| 对齐 | `__attribute__((aligned(64)))` | `alignas(64)` |
| 泛型 | `void*` + manual cast | `template<typename T>` |
| 线程 | `pthread_create/join` | `std::thread` + lambda |
| RAII | 手动 init/destroy | 构造/destruct 自动管理 |
| 编译期断言 | 无标准 | `static_assert` |
