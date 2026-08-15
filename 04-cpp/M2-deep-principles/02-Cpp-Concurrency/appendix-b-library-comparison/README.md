# 附录 B 并发程序库对比

**An Overview of C++ Concurrency Libraries**

## 本附录讲什么

C++ 标准库的并发设施是 C++11 才有的，之前大家用各种第三方库。本附录对比标准库、Boost、TBB、PPL、OpenMP、MPI 等主流并发方案，帮你在不同场景选对工具。

## 要点

### 主流并发库对比

| 库 | 层次 | 模型 | 优势 | 劣势 |
|----|------|------|------|------|
| C++ 标准库 (`<thread>`) | 语言级 | 线程+原子+future | 可移植、零依赖 | 线程池要自己写 |
| Boost.Thread | 语言级 | 同标准库（标准库的前身） | 跨版本兼容 | 依赖 Boost |
| Intel TBB | 库级 | 任务+数据并行+容器 | work-stealing、并发容器 | 仅 x86 优化最佳 |
| PPL (Microsoft) | 库级 | 任务并行 | 和 TBB 类似 | Windows 为主 |
| OpenMP | 编译指令 | 数据并行 | `#pragma` 一行并行化 | 不灵活、平台依赖 |
| MPI | 进程级 | 消息传递 | 跨节点分布式 | 进程间通信开销大 |
| ASIO | 库级 | 异步 IO + 协程 | 网络编程标配 | 非通用并行 |
| Folly (Facebook) | 库级 | 全栈 | 工业级无锁结构 | 依赖重 |

### C++ 标准库 vs TBB

**标准库**：
- 基础原语全（thread、mutex、atomic、future、cv）。
- 但**没有线程池、没有并发容器、没有 work-stealing**。
- C++17 并行算法依赖实现（MSVC 用 PPL、libstdc++ 可选 TBB）。

**TBB**：
- `tbb::parallel_for`、`parallel_reduce` 比手写分块简洁。
- `concurrent_vector`、`concurrent_hash_map` 现成可用。
- work-stealing 调度器，扩展性好。
- 缺点：额外的库依赖，非标准。

### OpenMP 的定位

```cpp
#pragma omp parallel for
for (int i = 0; i < n; ++i) {
    a[i] = b[i] + c[i];
}
```

- 一行 `#pragma` 就能并行化循环，极简。
- 适合科学计算、批量数据处理。
- 缺点：指令式不够灵活、调试困难、不是 C++ 标准（编译器扩展）。
- HFT 几乎不用——不确定性调度、不够精细控制。

### ASIO 与网络并发

ASIO 是 C++ 网络编程的事实标准（独立版 + `std::execution` 的基础）：
- **proactor 模型**：异步发起 IO，完成时回调。
- **io_context**：事件循环，单线程跑也能处理万级并发连接。
- **C++20 协程**：`co_await` 让异步代码写起来像同步。

HFT 行情网关用 ASIO 接 TCP/UDP feed，但**热路径收包用 DPDK 绕内核**，ASIO 只做管理通道。

### 消息传递 vs 共享内存

| 模型 | 代表 | 优势 | 劣势 |
|------|------|------|------|
| 共享内存 + 锁/原子 | C++ 标准库 | 性能高（同机） | 竞争、死锁风险 |
| 消息传递 | Erlang、Go channel、actor | 无共享、好理解 | 序列化开销 |
| CSP | Go、`std::channel`（C++26 提案） | 管道式组合 | 跨核拷贝 |

HFT 同机用共享内存 + SPSC 队列（消息传递的一种，但零拷贝），跨机用 UDP/RDMA。

### 选型建议

| 场景 | 推荐 |
|------|------|
| 通用多线程、可移植 | C++ 标准库 |
| 数据并行批处理 | TBB 或 C++17 并行算法 |
| 网络服务 | ASIO + 协程 |
| HFT 热路径 | 标准 `atomic` + 自写 SPSC 队列 + 绑核 |
| 科学计算 | OpenMP（快速原型）或 TBB |
| 分布式 | MPI 或 gRPC |

## HFT 关联

- **标准库为主**：HFT 热路径只用 `<atomic>` + `<thread>` + 自写无锁结构，避免引入 TBB/Folly 的不确定调度。
- **ASIO 做管理通道**：策略管理、监控、配置下发用 ASIO 异步 TCP，热路径收发包用 DPDK。
- **不用 OpenMP**：指令式并行不够精细，调度不可控，HFT 要求确定性延迟。
- **TBB 的 concurrent 容器慎用**：`concurrent_hash_map` 内部有细粒度锁，热路径不如自写无锁结构可控。
- **Folly 的无锁结构有参考价值**：`ProducerConsumerQueue`（SPSC）是 HFT 队列的工业级实现范本，可学习其 cache 行隔离技巧。
- **跨进程通信**：策略进程和行情网关进程用共享内存 + SPSC 队列，比 socket 快一个数量级。

## 自测题

1. C++ 标准库的并发设施相比 TBB 缺什么？为什么 HFT 仍倾向用标准库？
2. OpenMP 为什么不适合 HFT？它的优势场景是什么？
3. ASIO 的 proactor 模型是什么？HFT 中 ASIO 和 DPDK 分别负责什么？
4. 共享内存模型和消息传递模型的优劣对比？HFT 同机为什么用共享内存 + SPSC 队列？
5. Folly 的 `ProducerConsumerQueue` 有什么值得 HFT 学习的地方？

## 代码自测

### Q1: std::thread vs pthread
```cpp
// C++11
std::thread t([] { /* work */ });
t.join();

// POSIX
pthread_t t;
pthread_create(&t, nullptr,  -> void* { /* work */ return nullptr; }, nullptr);
pthread_join(t, nullptr);
```
> std::thread 相比 pthread 有哪些优势？

<details>
<summary>答案与复习指引</summary>

1. **类型安全**：`std::thread` 接受任意可调用对象（lambda、函数对象），pthread 只接受 `void*(*)(void*)`。
2. **跨平台**：`std::thread` 在 Windows/POSIX 都可用，pthread 仅 POSIX。
3. **RAII**：`std::thread` 析构时 joinable 会 terminate（强制处理），pthread 无此保护。
4. **移动语义**：`std::thread` 可移动存入容器，pthread_t 是原始句柄不可移动。
5. **参数传递**：`std::thread` 自动处理参数传递（值/引用/ref），pthread 需手动打包 `void*`。

**HFT 注意**：底层仍调 pthread（Linux），`std::thread` 额外开销极小。但 HFT 通常直接用 pthread 做更精细的绑核/调度策略控制。

**复习：** → [库比较](./README.md)
</details>
