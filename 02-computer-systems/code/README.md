# 02 CSAPP · 动手代码

与 **02 模块章节笔记** 对照；读 CSAPP 时在本目录编译运行。

每章提供 **C 和 C++ 配对**例子，对照同一概念在两种语言中的写法差异。

## Ch2 · 信息表示

| 文件 | CSAPP | 笔记 |
|------|-------|------|
| [ch02-endian-and-padding-demo.c](./ch02-endian-and-padding-demo.c) | Ch2 §2.1.2–2.1.3 + padding 预告 | [§2.1.2](../chapter-02-representing-information/notes/section-2.1.2-数据大小与sizeof.md) · [§2.1.3](../chapter-02-representing-information/notes/section-2.1.3-寻址与字节序.md) |
| [pointer-and-bytes.c](./pointer-and-bytes.c) | Ch2 §2.1.3 endian 逐字节 | [§2.1.3](../chapter-02-representing-information/notes/section-2.1.3-寻址与字节序.md) |
| [pointer-stride-demo.c](./pointer-stride-demo.c) | Ch3 §3.8 指针步长 | [§3.8 指针步长详解](../chapter-03-machine-level-programs/notes/section-3.8-指针步长详解.md) |

## Ch5 · 优化程序性能 🔴

| 文件 | 主题 | 笔记 |
|------|------|------|
| [ch05-optimization.c](./ch05-optimization.c) | 循环展开 + restrict + 累加器 (5 版渐进优化) | [§5.6](../chapter-05-optimizing-performance/notes/section-5.6-消除不必要的内存引用.md) · [§5.8](../chapter-05-optimizing-performance/notes/section-5.8-循环展开.md) |
| [ch05-optimization.cpp](./ch05-optimization.cpp) | 同上 + std::accumulate + 模板化 | 同上 |

## Ch6 · 存储器层次 🔴

| 文件 | 主题 | 笔记 |
|------|------|------|
| [ch06-cache-locality.c](./ch06-cache-locality.c) | 行/列遍历 + 分块 + AoS vs SoA | [§6.2](../chapter-06-memory-hierarchy/notes/section-6.2-局部性.md) |
| [ch06-cache-locality.cpp](./ch06-cache-locality.cpp) | 同上 + RAII Matrix + std::vector | 同上 |

## Ch9 · 虚拟内存 🔴

| 文件 | 主题 | 笔记 |
|------|------|------|
| [ch09-virtual-memory.c](./ch09-virtual-memory.c) | mmap + 页对齐 + mprotect + 大页 | [§9.8](../chapter-09-virtual-memory/notes/section-9.8-内存映射mmap.md) |
| [ch09-virtual-memory.cpp](./ch09-virtual-memory.cpp) | 同上 + RAII MappedMemory/MappedFile | 同上 |

## Ch11 · 网络编程 🔴

| 文件 | 主题 | 笔记 |
|------|------|------|
| [ch11-network-echo.c](./ch11-network-echo.c) | socket echo server + TCP_NODELAY | [§11.4](../chapter-11-network-programming/notes/section-11.4-套接字接口.md) |
| [ch11-network-echo.cpp](./ch11-network-echo.cpp) | 同上 + RAII SocketFd + std::thread | 同上 |

## Ch12 · 并发编程 🔴

| 文件 | 主题 | 笔记 |
|------|------|------|
| [ch12-concurrency.c](./ch12-concurrency.c) | 竞态 + mutex + atomic + 生产者-消费者 | [§12.3](../chapter-12-concurrent-programming/notes/section-12.3-基于线程的并发编程.md) · [§12.5](../chapter-12-concurrent-programming/notes/section-12.5-信号量与预线程化.md) |
| [ch12-concurrency.cpp](./ch12-concurrency.cpp) | 同上 + std::thread + lock_guard + BlockingQueue | 同上 |

## 编译

```bash
cd 02-computer-systems/code

# C
gcc -Wall -Wextra -std=c11 -O2 -o ch05_opt  ch05-optimization.c      -lm
gcc -Wall -Wextra -std=c11 -O2 -o ch06_cache ch06-cache-locality.c
gcc -Wall -Wextra -std=c11 -O2 -o ch09_vm    ch09-virtual-memory.c
gcc -Wall -Wextra -std=c11 -O2 -o ch11_echo  ch11-network-echo.c      -lpthread
gcc -Wall -Wextra -std=c11 -O2 -o ch12_conc  ch12-concurrency.c       -lpthread

# C++
g++ -Wall -Wextra -std=c++17 -O2 -o ch05_opt_cpp  ch05-optimization.cpp      -lm
g++ -Wall -Wextra -std=c++17 -O2 -o ch06_cache_cpp ch06-cache-locality.cpp
g++ -Wall -Wextra -std=c++17 -O2 -o ch09_vm_cpp    ch09-virtual-memory.cpp
g++ -Wall -Wextra -std=c++17 -O2 -o ch11_echo_cpp  ch11-network-echo.cpp      -lpthread
g++ -Wall -Wextra -std=c++17 -O2 -o ch12_conc_cpp  ch12-concurrency.cpp       -lpthread
```

## C vs C++ 对照要点

| 概念 | C | C++ |
|------|---|-----|
| restrict | `restrict` (C99) | `__restrict__` (GCC 扩展, 标准无) |
| 原子操作 | `_Atomic` / `stdatomic.h` | `std::atomic<T>` |
| 线程 | `pthread` | `std::thread` |
| 互斥 | `pthread_mutex_t` | `std::mutex` + `lock_guard` |
| 内存管理 | `malloc/free` | RAII / `unique_ptr` / `vector` |
| 对齐 | `__attribute__((aligned(64)))` | `alignas(64)` |
| 模板 | 无 | `template<typename T>` |
| 错误处理 | 返回值 / `errno` | 异常 `throw/catch` |
