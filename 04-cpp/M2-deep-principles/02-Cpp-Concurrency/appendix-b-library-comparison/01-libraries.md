# B.1 主流并发库对比

> 附录 B 并发程序库对比 · 上一章：[A.6 constexpr](../appendix-a-cpp11-primer/06-constexpr.md) · 下一节：[B.2 C++ 标准库 vs TBB](02-std-vs-tbb.md)

## 这节讲什么

C++ 标准库的并发设施是 C++11 才有的，之前大家用各种第三方库。本节对比标准库、Boost、TBB、PPL、OpenMP、MPI、ASIO、Folly 等主流并发方案，帮你在不同场景选对工具。

---

## 核心规则（代码+表格）

### 主流并发库全景

| 库 | 层次 | 模型 | 优势 | 劣势 |
|----|------|------|------|------|
| C++ 标准库 (`<thread>`) | 语言级 | 线程+原子+future | 可移植、零依赖 | 无线程池、无并发容器 |
| Boost.Thread | 语言级 | 同标准库 | 跨版本兼容 | 依赖 Boost |
| Intel TBB | 库级 | 任务+数据并行+容器 | work-stealing、并发容器 | 仅 x86 优化最佳 |
| PPL (Microsoft) | 库级 | 任务并行 | 和 TBB 类似 | Windows 为主 |
| OpenMP | 编译指令 | 数据并行 | `#pragma` 一行并行化 | 不灵活 |
| MPI | 进程级 | 消息传递 | 跨节点分布式 | 进程间通信开销大 |
| ASIO | 库级 | 异步 IO + 协程 | 网络编程标配 | 非通用并行 |
| Folly (Facebook) | 库级 | 全栈 | 工业级无锁结构 | 依赖重 |

### 选择决策树

```
需要跨节点分布式？ → MPI
需要网络编程？ → ASIO
需要并行 STL / 并行算法？ → TBB（或 C++17 并行 STL）
需要简单数据并行？ → OpenMP（#pragma）
需要工业级无锁结构？ → Folly
需要可移植、零依赖？ → C++ 标准库
需要跨版本兼容？ → Boost
只在 Windows？ → PPL
```

### 各库代码风格对比

```cpp
// C++ 标准库：手动管理
std::vector<std::thread> threads;
for (int i = 0; i < N; ++i)
    threads.emplace_back(worker, i);
for (auto& t : threads) t.join();

// OpenMP：一行指令
#pragma omp parallel for
for (int i = 0; i < N; ++i)
    worker(i);

// TBB：任务并行
tbb::parallel_for(0, N, [](int i){ worker(i); });

// C++17 并行 STL
std::for_each(std::execution::par, data.begin(), data.end(), worker);
```

### 并发原语覆盖度

| 原语 | 标准库 | Boost | TBB | OpenMP |
|------|--------|-------|-----|--------|
| thread | C++11 | Y | - | - |
| mutex | C++11 | Y | Y | Y(隐式) |
| atomic | C++11 | Y | Y | - |
| future | C++11 | Y | Y(task) | - |
| condition_variable | C++11 | Y | - | - |
| 线程池 | 无 | 无 | Y(内部) | Y(隐式) |
| 并发容器 | 无 | 无 | Y | - |
| 并行算法 | C++17 | 无 | Y | Y |
| work-stealing | 无 | 无 | Y | Y(隐式) |
| 协程 | C++20 | Y | - | - |

---

## 新手要点（和 C 的区别）

- **C 程序员可能只用过 pthread + OpenMP**：C 的并发世界主要是 pthread（POSIX）和 OpenMP（编译指令）。C++ 生态更丰富——TBB、ASIO、Folly 都是 C++ 生态的产物。
- **"标准库够用吗"是常见疑问**：C 程序员可能觉得 C++ 标准库没有线程池、没有并发容器——确实。标准库提供原语（thread/mutex/atomic），但高级结构要靠 TBB 或自己写。C++26 可能引入标准线程池。
- **OpenMP 和 C++ 并行 STL 的选择**：C 程序员如果熟悉 OpenMP，可以在 C++ 继续用。但 C++17 并行 STL 更"C++风格"（用迭代器而非 pragma）。功能类似，选择看团队偏好。
- **ASIO 是 C++ 网络编程的事实标准**：C 程序员做网络可能用 libevent/libuv——C++ 生态中 ASIO 是标准选择，且支持协程（C++20）。

---

## HFT 关联

- **HFT 热路径用标准库 + 自研**：HFT 热路径不能用 TBB（不确定性调度），用 C++ 标准库的 atomic + 自研 SPSC 队列。
- **TBB 用于盘后**：盘后批处理用 TBB 的 `parallel_for`/`concurrent_hash_map`——比手写高效。
- **ASIO 用于网关**：HFT 的 TCP 网关（连接交易所）用 ASIO——异步 IO + 协程。
- **Folly 的无锁结构**：Facebook 的 Folly 提供工业级无锁队列（`MPMCQueue`、`HazPtr`）——HFT 可以参考或直接使用（但依赖重）。
- **避免 MPI**：HFT 是单机低延迟，不用 MPI（跨节点分布式通信开销太大）。

---

## 自测题

1. C++ 标准库的并发设施相比 TBB 缺什么？
2. OpenMP 和 C++17 并行 STL 有什么区别？各有什么优缺点？
3. ASIO 适合什么场景？HFT 中用在哪里？
4. HFT 热路径为什么用标准库+自研而非 TBB？
5. MPI 为什么不适合 HFT？

---

## 参考与延伸

- 下一节：[B.2 C++ 标准库 vs TBB](02-std-vs-tbb.md)
- 上一章：[A.6 constexpr](../appendix-a-cpp11-primer/06-constexpr.md)
- 回到：[附录 B](README.md)
