# B.2 C++ 标准库 vs TBB

> 附录 B · 上一节：[B.1 主流并发库对比](01-libraries.md) · 下一节：[B.3 OpenMP 的定位](03-openmp.md)

## 这节讲什么

C++ 标准库提供并发原语，Intel TBB 提供高级并发结构。本节深入对比两者——标准库缺什么、TBB 补什么、以及什么场景该用哪个。

---

## 核心规则（代码+表格）

### 标准库有什么

| 类别 | 内容 |
|------|------|
| 线程 | `thread`、`jthread`(C++20) |
| 互斥 | `mutex`、`recursive_mutex`、`shared_mutex`、`timed_mutex` |
| RAII | `lock_guard`、`unique_lock`、`scoped_lock`、`shared_lock` |
| 原子 | `atomic<T>`、`atomic_flag`、`ref` |
| 同步 | `condition_variable`、`condition_variable_any` |
| 异步 | `future`、`promise`、`packaged_task`、`async` |
| 屏障 | `latch`、`barrier`、`counting_semaphore`(C++20) |
| 并行算法 | `execution::seq/par/par_unseq`(C++17) |

### 标准库缺什么

| 缺失 | 说明 | 替代 |
|------|------|------|
| 线程池 | 无标准线程池 | TBB / 自研 / C++26 |
| 并发容器 | 无 `concurrent_vector` 等 | TBB / 自研分段锁 |
| work-stealing | 无标准调度器 | TBB |
| 任务图 | 无 DAG 调度 | TBB Flow Graph |
| 无锁结构 | 无标准无锁队列 | 自研 / Folly |
| 协程库 | 只有语言设施(C++20) | ASIO / cppcoro |

### TBB 补什么

```cpp
// 1. 并行算法（比标准库更丰富）
tbb::parallel_for(0, N, [&](int i){ work(i); });
tbb::parallel_reduce(range, init, func, reduction);

// 2. 并发容器
tbb::concurrent_vector<int> cv;
cv.push_back(42);  // 并发安全 push
tbb::concurrent_hash_map<K, V> chm;
chm.insert({key, val});

// 3. 任务调度
tbb::task_group g;
g.run([]{ task1(); });
g.run([]{ task2(); });
g.wait();  // 等所有任务完成

// 4. Flow Graph（任务 DAG）
tbb::flow::graph graph;
auto node1 = tbb::flow::continue_node<...>(graph, ...);
auto node2 = tbb::flow::continue_node<...>(graph, ...);
tbb::flow::make_edge(node1, node2);  // node1 → node2
graph.wait_for_all();
```

### 性能对比

| 维度 | 标准库 + 手写 | TBB |
|------|-------------|-----|
| 线程池性能 | 手写通常较粗糙 | work-stealing，工业优化 |
| 并行算法 | 手写分块 | 自动分块+负载均衡 |
| 并发容器 | 手写分段锁 | 细粒度锁/无锁，高度优化 |
| 启动开销 | 线程创建 ~10-50μs | 任务提交 ~1μs（池化） |
| 依赖 | 零 | 需链接 TBB |

### 什么时候用 TBB

```cpp
// 场景1：需要并发容器
tbb::concurrent_hash_map<std::string, Config> config_map;
// 多线程读写配置 → TBB 的并发哈希表比手写分段锁更好

// 场景2：需要并行递归（fork/join）
tbb::parallel_sort(data.begin(), data.end());  // 并行快排
// 比手写 std::thread 分块 + join 更高效

// 场景3：需要任务依赖图
// 解析→策略→风控→下单，有依赖关系
tbb::flow::graph g;
// ... 构建 DAG ...
g.wait_for_all();
```

### 什么时候用标准库

```cpp
// 场景1：简单线程创建
std::thread t(worker);
t.join();

// 场景2：需要精确控制（HFT 绑核）
// TBB 不暴露线程亲和性 → 标准库 + pthread_setaffinity_np

// 场景3：零依赖要求
// 不能引入 TBB 依赖 → 标准库 + 自研

// 场景4：低并发
// 线程数少、竞争小 → 标准库够用
```

---

## 新手要点（和 C 的区别）

- **C 程序员可能觉得"标准库就够了"**：C 的标准库（C11）连 thread 都没有（`<threads.h>` 几乎没实现）。C++ 标准库已经比 C 丰富很多——但和 TBB 比仍缺高级结构。C 程序员转型 C++ 时要了解"TBB 补了什么"。
- **"自己写还是用 TBB"是常见决策**：C 程序员可能习惯"自己写"——但 TBB 的并发容器和 work-stealing 经过工业验证，自己写很难超越。除非有特殊需求（如 HFT 绑核），优先用 TBB。
- **TBB 是 C++17 并行 STL 的后端**：GCC 的 `std::execution::par` 底层用 TBB——即使你写标准库代码，也可能间接依赖 TBB。
- **依赖管理**：C 程序员可能不习惯 C++ 的库依赖管理——TBB 需要安装（`apt install libtbb-dev`）和链接（`-ltbb`）。部署时要注意。

---

## HFT 关联

- **HFT 热路径用标准库**：原子操作、SPSC 队列用标准库——TBB 的不确定性调度不适合热路径。
- **HFT 盘后用 TBB**：回测、因子计算用 TBB 的 `parallel_for`、`concurrent_hash_map`——比手写高效。
- **HFT 的配置管理用 TBB 并发容器**：多线程读写配置表，用 `tbb::concurrent_hash_map`——比手写分段锁更可靠。
- **TBB 版本兼容性**：HFT 生产环境如果用 TBB，要锁定版本——TBB 升级可能改 ABI。

---

## 自测题

1. C++ 标准库相比 TBB 缺哪些关键设施？
2. TBB 的 `concurrent_hash_map` 比手写分段锁好在哪？
3. 什么场景应该用 TBB 而非标准库？什么场景相反？
4. 为什么 HFT 热路径用标准库而非 TBB？
5. C++17 的 `std::execution::par` 和 TBB 有什么关系？

---

## 参考与延伸

- 下一节：[B.3 OpenMP 的定位](03-openmp.md)
- 上一节：[B.1 主流并发库对比](01-libraries.md)
- 回到：[附录 B](README.md)
