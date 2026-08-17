# B.5 消息传递 vs 共享内存

> 附录 B · 上一节：[B.4 ASIO 与网络并发](04-asio.md) · 下一节：[B.6 选型建议](06-recommendations.md)

## 这节讲什么

并发编程有两大范式：**共享内存**（线程间通过共享变量+锁通信）和**消息传递**（线程间通过消息队列通信，不共享可变状态）。本节对比两者、讲各自的优缺点，以及为什么 HFT 偏好消息传递。

---

## 核心规则（代码+表格）

### 两种范式对比

| 维度 | 共享内存 | 消息传递 |
|------|----------|----------|
| 通信方式 | 共享变量 + 锁/原子 | 消息队列（SPSC/MPMC） |
| 同步 | 显式（mutex/atomic） | 隐式（队列本身就是同步） |
| 耦合度 | 高（共享状态） | 低（只通过消息交互） |
| 可调试性 | 差（竞争/死锁） | 好（消息可记录/回放） |
| 性能 | 高竞争时差 | 无共享时优 |
| 扩展性 | 差（共享是瓶颈） | 优（无共享=线性扩展） |
| 分布式 | 不支持（单机） | 天然支持 |

### 共享内存示例

```cpp
// 共享内存：多线程操作同一个数据结构
class SharedOrderBook {
    std::mutex m;
    std::map<Price, Volume> bids, asks;
public:
    void update(Price p, Volume v, bool is_bid) {
        std::lock_guard<std::mutex> lk(m);
        if (is_bid) bids[p] = v;
        else asks[p] = v;
    }
    Price get_mid() const {
        std::lock_guard<std::mutex> lk(m);
        return (bids.begin()->first + asks.begin()->first) / 2;
    }
};
// 问题：每次操作都争锁，高并发下瓶颈
```

### 消息传递示例

```cpp
// 消息传递：线程间通过队列通信，不共享可变状态

// 行情线程拥有 OrderBook，不共享
class MarketDataThread {
    OrderBook book;  // 只有本线程访问
    SPSCQueue<Tick> incoming;   // 从网卡线程接收
    SPSCQueue<Snapshot> outgoing;  // 发给策略线程
public:
    void run() {
        for (;;) {
            Tick t;
            if (incoming.pop(t)) {
                book.update(t);  // 无锁，独占
                Snapshot s = book.get_snapshot();
                outgoing.push(s);  // 发给策略线程
            }
        }
    }
};

// 策略线程拥有自己的状态
class StrategyThread {
    SPSCQueue<Snapshot> incoming;  // 从行情线程接收
    SPSCQueue<Order> outgoing;     // 发给下单线程
public:
    void run() {
        for (;;) {
            Snapshot s;
            if (incoming.pop(s)) {
                Order o = compute(s);  // 无锁，独占
                outgoing.push(o);
            }
        }
    }
};
// 每个线程独占自己的数据，通过队列通信 → 无锁、无竞争
```

### Actor 模型

```
Actor 模型是消息传递的极致形式：
  - 每个 Actor 有自己的私有状态
  - Actor 之间只能通过消息通信
  - 没有共享可变状态
  - Erlang/Akka 的核心模型

HFT 的流水线架构 = Actor 模型的简化版：
  每个线程 = 一个 Actor
  SPSC 队列 = Actor 的 mailbox
```

### 什么时候用共享内存

```cpp
// 1. 只读共享：配置表、常量
const Config& config = get_config();  // 启动后只读 → 无竞争

// 2. 低并发场景
// 线程数少、操作频率低 → 锁竞争不严重

// 3. 需要细粒度共享
// 如多个线程需要原子地读写同一个计数器
std::atomic<int> connection_count{0};
```

### 什么时候用消息传递

```cpp
// 1. 高并发热路径（HFT）
// 每个线程独立处理，通过队列传递 → 无竞争

// 2. 流水线架构
// 采集→解析→策略→下单，每阶段一个线程

// 3. 需要可回放/可调试
// 消息可以记录，事后回放重现问题

// 4. 未来可能分布式
// 消息传递天然支持跨进程/跨机器
```

---

## 新手要点（和 C 的区别）

- **C 程序员通常用共享内存**：C 的多线程通常是 pthread + 全局变量 + mutex——这是共享内存范式。消息传递在 C 里不常见（要自己写队列或用第三方库）。
- **"不共享可变状态"是思维转变**：C 程序员可能觉得"共享+锁很自然"——但消息传递的"每线程独占数据"更安全、更好调试。C 程序员转型 HFT 时要改掉"全局变量+锁"的习惯。
- **Actor 模型是 C 程序员陌生的概念**：C 没有 Actor 模型。Erlang/Akka 是 Actor 模型的代表。C++ 的 SPSC 队列流水线是 Actor 模型的简化——每个线程像一个 Actor。
- **消息传递的性能优势**：C 程序员可能觉得"队列传递有开销"——但 SPSC 无锁队列的开销极小（一次原子 store/load），远小于锁竞争。在高并发下消息传递更快。

---

## HFT 关联

- **HFT 是消息传递的典型应用**：HFT 的流水线架构（采集→解析→策略→下单）就是消息传递——每线程独占数据，SPSC 队列传递消息。这是 HFT 低延迟的核心设计。
- **"零共享"原则**：HFT 系统设计的第一原则是"线程间不共享可变状态"——所有数据通过 SPSC 队列传递。这消除了锁竞争和 cache bounce。
- **消息可回放 = 可调试**：HFT 系统的消息（tick、order）被记录到日志——出问题时可以回放消息流，精确重现。共享内存方式无法做到这点。
- **共享内存只用于只读**：HFT 的配置表、合约信息（启动后不变）可以共享——因为是只读，无竞争。可变状态一律用消息传递。

---

## 自测题

1. 共享内存和消息传递各有什么优缺点？
2. 为什么消息传递的可扩展性比共享内存好？
3. HFT 的流水线架构为什么是消息传递的典型应用？
4. 什么是 Actor 模型？它和 HFT 的 SPSC 队列有什么关系？
5. 为什么 HFT 的"零共享"原则能降低延迟？

---

## 参考与延伸

- 下一节：[B.6 选型建议](06-recommendations.md)
- 上一节：[B.4 ASIO 与网络并发](04-asio.md)
- 回到：[附录 B](README.md)
