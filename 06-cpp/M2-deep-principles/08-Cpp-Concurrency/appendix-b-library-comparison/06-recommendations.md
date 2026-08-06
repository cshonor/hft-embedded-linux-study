# B.6 选型建议

> 附录 B · 上一节：[B.5 消息传递 vs 共享内存](05-msg-vs-shared.md) · 下一章：[附录 C.1 ATM 状态机设计](../appendix-c-atm-example/01-state-machine.md)

## 这节讲什么

综合前 5 节的内容，给出不同场景下的并发库/范式选型建议。本节是一个决策指南——根据"你的需求是什么"推荐"该用什么"。

---

## 核心规则（代码+表格）

### 按场景选型

| 场景 | 推荐 | 理由 |
|------|------|------|
| HFT 热路径 | 标准库 + 自研 SPSC | 零共享、绑核、确定性 |
| HFT 盘后批处理 | TBB / C++17 并行 STL | 并行算法、负载均衡 |
| HFT 网关 | ASIO + 协程 | 异步 IO、多连接 |
| 通用服务器 | 标准库 + 线程池 | 可移植、零依赖 |
| 科学计算 | OpenMP / TBB | 数据并行、简洁 |
| 分布式系统 | MPI / gRPC | 跨节点通信 |
| 高并发 Web | ASIO / Folly | 工业级优化 |
| 遗留 C 迁移 | OpenMP + 标准库 | 最小改动 |

### 按需求选型

```
需求：极致低延迟（纳秒级）
  → 标准库 atomic + 自研 SPSC + 绑核
  → 避免：TBB（调度不确定）、mutex（竞争时阻塞）、malloc（锁）

需求：高吞吐（非低延迟）
  → TBB 并行算法 + 并发容器
  → 或 C++17 并行 STL

需求：网络编程
  → ASIO + C++20 协程
  → 避免：手写 epoll（ASIO 已封装）

需求：简单并行化循环
  → OpenMP（#pragma 一行搞定）
  → 或 C++17 并行 STL

需求：零依赖
  → 标准库 + 自研
  → 避免：TBB、Boost、Folly

需求：分布式
  → MPI（HPC）或 gRPC（微服务）
  → 避免：在单机库上强行分布式

需求：无锁结构
  → 自研（学习目的）
  → 或 Folly（工业级）
  → 避免：无经验时自研生产用无锁结构
```

### 混合策略

```cpp
// 实际项目通常混合多种技术
class HFTSystem {
    // 热路径：标准库 + 自研
    SPSCQueue<Tick> md_queue;  // 自研无锁队列
    std::atomic<bool> running{true};  // 标准库原子

    // 盘后：TBB
    void backtest() {
        tbb::parallel_for(0, num_symbols, [&](int i){
            backtest_symbol(i);
        });
    }

    // 网关：ASIO
    asio::io_context gateway_ctx;
    void run_gateway() {
        asio::co_spawn(gateway_ctx, listen(8080), asio::detached);
        gateway_ctx.run();
    }

    // 配置：只读共享
    const Config config;  // 启动后只读，无竞争
};
```

### 避坑清单

| 坑 | 建议 |
|----|------|
| 热路径用 mutex | 用 SPSC 队列或 atomic |
| 热路径用 malloc | 用 mempool |
| 热路径用 shared_ptr | 用 unique_ptr 或裸指针 |
| 热路径用 TBB | 用标准库 + 自研 |
| 无锁结构生产用 | 先在 TSan + 压测验证 |
| 全局变量 + 锁 | 改消息传递 |
| volatile 做同步 | 用 atomic |
| DCLP | 用 Meyers Singleton |
| thread 忘记 join | 用 jthread |
| 回调地狱 | 用协程 |

---

## 新手要点（和 C 的区别）

- **C 程序员的选型通常简单：pthread + 全局锁**：C 生态的并发选择不多。C++ 生态丰富——选择多但要懂得取舍。C 程序员转型 C++ 时要学习"什么场景用什么工具"。
- **"零依赖"在 HFT 中可能很重要**：C 程序员可能习惯链接各种库——但 HFT 生产环境可能要求最小依赖（便于部署、ABI 稳定）。标准库 + 自研是 HFT 的常见选择。
- **混合策略是常态**：C 程序员可能想"一个库解决所有问题"——但实际项目通常混合多种技术。HFT 系统的热路径用标准库、盘后用 TBB、网关用 ASIO——各取所长。
- **"避免"清单比"推荐"更重要**：知道"不该用什么"比"该用什么"更关键——比如热路径避免 mutex/malloc/shared_ptr，这些避坑知识来自经验。

---

## HFT 关联

- **HFT 的选型哲学：热路径极简，非热路径用轮子**：热路径只标准库+自研（可控、零开销），非热路径用 TBB/ASIO（高效、省开发成本）。
- **依赖管理**：HFT 生产环境依赖越少越好——TBB/ASIO 如果用，要锁定版本、静态链接。避免动态链接的 ABI 兼容问题。
- **"先正确再优化"**：HFT 开发先用标准库+TBB 实现正确版本，benchmark 找瓶颈，再对热路径手写优化。不要一开始就全部自研。
- **团队技能匹配**：选型要考虑团队技能——如果团队无无锁编程经验，不要自研无锁结构，用 Folly 或有锁方案。

---

## 自测题

1. HFT 热路径应该用什么并发库？避免什么？
2. HFT 盘后批处理应该用什么？为什么不用热路径的方案？
3. 什么场景适合用 OpenMP？什么场景适合用 C++17 并行 STL？
4. 为什么实际项目通常混合多种并发技术？举一个 HFT 的例子。
5. HFT 开发应该"先正确再优化"还是"一开始就极致优化"？

---

## 参考与延伸

- 下一章：[附录 C.1 ATM 状态机设计](../appendix-c-atm-example/01-state-machine.md)
- 上一节：[B.5 消息传递 vs 共享内存](05-msg-vs-shared.md)
- 回到：[附录 B](README.md)
