# P8 — 迷你撮合引擎（终极大作业）

> 实现一个限价订单簿撮合引擎：无锁 ring buffer + 绑核/Hugepage + Rust 重写。把前面所有模块的能力收口到一个 HFT 核心组件。
> **做法：项目驱动，[`18`](../../14-hft-engineering/) / [`21`](../../18-rust-quant/) / [`22`](../../19-markets-microstructure/) 笔记当字典。**

---

## 核心理念

这是整条学习路线的终局——一个能跑、能测、能剖的撮合引擎。C++ 版沉淀工程能力，Rust 版验证内存安全与零成本抽象。**每一行代码都能对应到前面某个模块学过的原理。**

## 最小预备

| 瞄一眼 | 只要留下印象 |
|--------|-------------|
| [HFT ch02 撮合原理](../../14-hft-engineering/chapter-02-交易所架构与撮合原理/README.md) | 价格优先 + 时间优先 |
| [HFT ch03 订单簿](../../14-hft-engineering/chapter-03-订单簿深度与行情解析/README.md) | LOB = 买单簿 + 卖单簿 |
| [HFT ch07 无锁数据结构](../../14-hft-engineering/chapter-07-无锁数据结构与内存布局/README.md) | SPSC ring buffer、缓存行对齐 |
| [HFT ch08 核心引擎](../../14-hft-engineering/chapter-08-超低延迟核心引擎开发/README.md) | 绑核/大页/mlock |
| [HFT ch09 延迟测量](../../14-hft-engineering/chapter-09-延迟测量与基准压测/README.md) | p50/p99/p999 |
| [Harris 订单类型](../../19-markets-microstructure/) | 限价单/市价单/IOC/FOK |

---

## Phase 1：LOB 数据结构 + 限价单撮合（2-3 小时）

### 做什么

实现限价订单簿（LOB）数据结构，支持限价单下单和撮合。单线程，先保证正确性。

### 核心概念

```
买单簿 (Bids)              卖单簿 (Asks)
Price   Quantity            Price   Quantity
102.00  100                 103.00  200
101.00  300  ← 买二         104.00  150
100.00  500  ← 买一（最优）  105.00  400

最优买价 = 100.00   最优卖价 = 103.00   买卖价差 = 3.00
```

撮合规则：
- **价格优先**：买价高的先成交，卖价低的最先成交
- **时间优先**：同价位先到的先成交（FIFO）

### 代码骨架

```cpp
// src/lob.hpp
#include <map>
#include <list>
#include <cstdint>
#include <vector>

using Price = int64_t;      // 价格用整数（×10000 避免浮点）
using Qty   = int64_t;

enum class Side { Buy, Sell };

struct Order {
    uint64_t id;
    Side side;
    Price price;
    Qty quantity;
    uint64_t timestamp;  // 时间优先用
};

// 每个价位一个队列（FIFO）
struct PriceLevel {
    std::list<Order> orders;
    Qty total_qty = 0;
};

// LOB = 买单簿 + 卖单簿
class OrderBook {
    // 买单：降序（最高价在前面）
    std::map<Price, PriceLevel, std::greater<Price>> bids;
    // 卖单：升序（最低价在前面）
    std::map<Price, PriceLevel, std::less<Price>> asks;

public:
    struct Trade {
        uint64_t buy_order_id;
        uint64_t sell_order_id;
        Price price;
        Qty quantity;
    };

    // 下限价单 → 返回成交列表
    std::vector<Trade> add_limit_order(Order order) {
        std::vector<Trade> trades;

        if (order.side == Side::Buy) {
            // 买单：尝试吃对手卖单簿（从最低卖价开始）
            while (order.quantity > 0 && !asks.empty()) {
                auto& [ask_price, level] = *asks.begin();
                if (order.price < ask_price) break;  // 买价不够，不成交

                while (order.quantity > 0 && !level.orders.empty()) {
                    auto& sell_order = level.orders.front();
                    Qty match_qty = std::min(order.quantity, sell_order.quantity);

                    trades.push_back({order.id, sell_order.id, ask_price, match_qty});
                    order.quantity -= match_qty;
                    sell_order.quantity -= match_qty;
                    level.total_qty -= match_qty;

                    if (sell_order.quantity == 0)
                        level.orders.pop_front();
                }
                if (level.orders.empty())
                    asks.erase(asks.begin());
            }
            // 剩余量挂单
            if (order.quantity > 0)
                bids[order.price].orders.push_back(order);
        } else {
            // 卖单：对称逻辑，吃买单簿
            // ... (同上，方向相反)
        }
        return trades;
    }

    Price best_bid() const { return bids.empty() ? 0 : bids.begin()->first; }
    Price best_ask() const { return asks.empty() ? 0 : asks.begin()->first; }
};
```

### 分步实现

1. **定义 Order 结构**：id/side/price/quantity/timestamp
2. **用 `std::map` 管理价位**：买单 `greater`（降序），卖单 `less`（升序）
3. **每个价位用 `std::list<Order>`**：FIFO 队列，头删尾插 O(1)
4. **撮合逻辑**：买单来时，从最低卖价开始吃；卖单来时，从最高买价开始吃
5. **测试**：
   - 挂卖单 103@200 → 挂买单 105@100 → 成交 100@103
   - 挂买单 100@500 → 挂卖单 98@300 → 成交 300@100
   - 验证价格优先 + 时间优先

### 常见坑

| 坑 | 症状 | 原因 |
|----|------|------|
| 价格用 double | 精度丢失 | 用整数（price × 10000） |
| map 迭代器失效 | 段错误 | erase 后迭代器失效，要用返回值 |
| 忘了时间戳排序 | 同价位成交顺序错 | list 本身是 FIFO，push_back 保证时间优先 |
| 自成交 | 逻辑漏洞 | 要检查买价 ≥ 卖价才成交 |

### 卡住翻哪篇笔记

| 卡住了… | 翻这里 |
|---------|--------|
| 撮合规则 | [HFT ch02](../../14-hft-engineering/chapter-02-交易所架构与撮合原理/README.md) |
| 订单簿结构 | [HFT ch03](../../14-hft-engineering/chapter-03-订单簿深度与行情解析/README.md) |
| 订单类型 | [Harris](../../19-markets-microstructure/) |

---

## Phase 2：市价单 + IOC/FOK + 成交回报（1-2 小时）

### 做什么

加市价单（不指定价格，立即成交）和 IOC（立即成交剩余撤销）/FOK（全部成交否则撤销）。

### 代码骨架

```cpp
enum class OrderType { Limit, Market, IOC, FOK };

// 市价单：吃对手簿直到吃完或数量为 0
std::vector<Trade> add_market_order(Order order) {
    std::vector<Trade> trades;
    // 跟限价单撮合逻辑一样，但没有价格检查（直接吃）
    // 买单吃卖单簿，卖单吃买单簿
    // ...
    return trades;
}

// IOC：限价单，剩余立即撤销
// FOK：限价单，如果不能全部成交就全部撤销
std::vector<Trade> add_ioc_order(Order order) {
    auto trades = match_limit(order);  // 先撮合
    // 剩余量不挂单，直接丢弃
    return trades;
}

std::vector<Trade> add_fok_order(Order order) {
    // 先检查能不能全部成交
    Qty available = check_available_quantity(order);
    if (available < order.quantity) {
        return {};  // 不能全部成交，全部撤销
    }
    return match_limit(order);  // 能全部成交，执行
}
```

### 分步实现

1. **市价单**：跟限价单一样吃对手簿，但不检查价格（`order.price = INT_MAX` for buy）
2. **IOC**：撮合后剩余量不挂单
3. **FOK**：先预扫描能成交多少量，不够就全部撤销
4. **成交回报**：每次成交生成 Trade 结构，推到输出队列

### 卡住翻哪篇笔记

| 卡住了… | 翻这里 |
|---------|--------|
| 订单类型定义 | [Harris](../../19-markets-microstructure/) |

---

## Phase 3：无锁 ring buffer 衔接行情输入（1-2 小时）

### 做什么

用 P2.5 的 SPSC 无锁 ring buffer 接行情输入，撮合引擎从队列消费订单。

### 分步实现

1. **复用 P2.5 交付物 3 的 ringbuf**：直接拷代码过来
2. **生产者**：网络线程解析 UDP 组播行情 → 构造 Order → `ringbuf_push`
3. **消费者**：撮合线程 `ringbuf_pop` → 撮合 → 推成交回报
4. **缓存行对齐**：`struct OrderBook __attribute__((aligned(64)))` 防伪共享
5. **测试**：生产者猛推 100 万订单，消费者撮合，验证无丢无重

### 卡住翻哪篇笔记

| 卡住了… | 翻这里 |
|---------|--------|
| 无锁 ring buffer | P2.5 交付物 3（你写的代码！）|
| 无锁原理 | [HFT ch07](../../14-hft-engineering/chapter-07-无锁数据结构与内存布局/README.md) |
| memory order | [C++ Concurrency ch05](../../04-cpp/M4-deep-principles/02-Cpp-Concurrency/ch05-memory-model-atomics/) |

---

## Phase 4：绑核/大页/mlock + 延迟基准（1-2 小时）

### 做什么

把撮合引擎绑到独占核，用大页 + mlock 锁定内存，测单笔撮合延迟。

### 代码骨架

```cpp
#include <sched.h>
#include <sys/mman.h>

// 绑核
void pin_to_cpu(int cpu) {
    cpu_set_t cpuset;
    CPU_ZERO(&cpuset);
    CPU_SET(cpu, &cpuset);
    sched_setaffinity(0, sizeof(cpuset), &cpuset);
}

// 实时优先级
void set_realtime_priority() {
    struct sched_param param = { .sched_priority = 99 };
    sched_setscheduler(0, SCHED_FIFO, &param);
}

// 锁定内存（防 page fault）
void lock_memory() {
    mlockall(MCL_CURRENT | MCL_FUTURE);
}

int main() {
    pin_to_cpu(2);           // 绑到核 2（isolcpus 隔离的核）
    set_realtime_priority(); // SCHED_FIFO 优先级 99
    lock_memory();           // 锁定所有内存页

    // 预热：让所有页都在 TLB 里
    for (int i = 0; i < 10000; i++) {
        run_one_match();
    }

    // 延迟基准
    uint64_t latencies[1000000];
    for (int i = 0; i < 1000000; i++) {
        uint64_t t1 = rdtsc();   // P2.5 交付物 8 的时间戳读取
        run_one_match();
        uint64_t t2 = rdtsc();
        latencies[i] = t2 - t1;
    }

    // 排序算 p50/p99/p999
    std::sort(latencies, latencies + 1000000);
    printf("p50: %llu cycles\n", latencies[500000]);
    printf("p99: %llu cycles\n", latencies[990000]);
    printf("p999: %llu cycles\n", latencies[999000]);
}
```

### 分步实现

1. **`sched_setaffinity`** 绑核：绑到 `isolcpus` 隔离的核
2. **`SCHED_FIFO` 优先级 99**：不让普通进程抢占
3. **`mlockall`**：锁定内存，防运行中 page fault
4. **大页**：`mmap(MAP_HUGETLB)` 分配 LOB 数据结构
5. **预热**：跑 1 万次让缓存热起来，再开始测
6. **基准**：测 100 万次单笔撮合，算 p50/p99/p999

### 常见坑

| 坑 | 症状 | 原因 |
|----|------|------|
| 没 isolcpus | p99 不稳定 | 其他进程抢核 |
| page fault | 偶发高延迟 | mlockall 不够，还要预触摸页 |
| TLB miss | 延迟高 | 用大页减少 TLB miss |
| 测量本身有开销 | 延迟偏高 | rdtsc 比 clock_gettime 开销小 |

### 卡住翻哪篇笔记

| 卡住了… | 翻这里 |
|---------|--------|
| 绑核/大页/mlock | [HFT ch05](../../14-hft-engineering/chapter-05-操作系统内核极致调优/README.md) |
| 延迟测量 | [HFT ch09](../../14-hft-engineering/chapter-09-延迟测量与基准压测/README.md) |
| 内存布局 | [HFT ch07](../../14-hft-engineering/chapter-07-无锁数据结构与内存布局/README.md) |

---

## Phase 5：perf/bpftrace 剖析 + 优化尾延迟（1-2 小时）

### 做什么

用 perf 火焰图找热点，用 bpftrace 找尾延迟源，优化一轮。

### 分步实现

1. **perf 火焰图**：
   ```bash
   sudo perf record -F 99 -g -p $(pgrep matching_engine) -- sleep 10
   # 生成火焰图（同 P7 Phase 4）
   ```
   → 看 `add_limit_order` 里哪个函数最宽？`std::map::find`？内存分配？

2. **优化方向**：
   - `std::map` → 换成扁平数组（价格范围固定时用 `price → level` 直接索引）
   - `std::list` → 换成侵入式链表（P2.5 交付物 2，减少 malloc）
   - `new/delete` → 预分配 + 内存池
   - 分支预测：`likely`/`unlikely` 标注热路径

3. **bpftrace 找抖动**：
   ```bash
   sudo bpftrace -e '
   tracepoint:sched:sched_switch /args->prev_pid == PID/ {
       printf("%lld scheduled out (latency spike!)\n", nsecs);
   }
   '
   ```

4. **对比优化前后**：p99 应该明显下降

### 卡住翻哪篇笔记

| 卡住了… | 翻这里 |
|---------|--------|
| perf/火焰图 | P7 Phase 4（你已经做过！）|
| bpftrace 抖动 | P7 Phase 5（同上）|
| HFT 调优 | [HFT ch05](../../14-hft-engineering/chapter-05-操作系统内核极致调优/README.md) |

---

## Phase 6：Rust 重写 + 对比（2-3 小时）

### 做什么

用 Rust 重写撮合引擎，验证内存安全与零成本抽象。

### 代码骨架

```rust
// src/lob.rs
use std::collections::BTreeMap;

#[derive(Clone, Copy, PartialEq)]
enum Side { Buy, Sell }

#[derive(Clone)]
struct Order {
    id: u64,
    side: Side,
    price: i64,
    quantity: i64,
    timestamp: u64,
}

struct Trade {
    buy_order_id: u64,
    sell_order_id: u64,
    price: i64,
    quantity: i64,
}

struct OrderBook {
    // 买单：降序（BTreeMap 默认升序，用 Reverse）
    bids: BTreeMap<std::cmp::Reverse<i64>, Vec<Order>>,
    asks: BTreeMap<i64, Vec<Order>>,
}

impl OrderBook {
    fn add_limit_order(&mut self, order: Order) -> Vec<Trade> {
        let mut trades = Vec::new();
        let mut remaining = order.quantity;

        match order.side {
            Side::Buy => {
                while remaining > 0 {
                    if let Some((&ask_price, orders)) = self.asks.first_key_value() {
                        if order.price < ask_price { break; }
                        // 撮合...
                    } else { break; }
                }
            }
            Side::Sell => { /* 对称 */ }
        }
        trades
    }
}

// 无锁队列用 unsafe（唯一允许 unsafe 的地方）
mod ringbuf {
    use std::sync::atomic::{AtomicUsize, Ordering};

    pub struct RingBuffer<T: Copy> {
        buffer: *mut T,
        capacity: usize,
        head: AtomicUsize,
        tail: AtomicUsize,
    }
    // unsafe 实现 push/pop（跟 C 版逻辑一样）
}
```

### 分步实现

1. **用 `BTreeMap` 替代 `std::map`**：Rust 的 BTreeMap = C++ 的 map
2. **用 `Vec<Order>` 替代 `std::list`**：Rust 没有 intrusive list，用 Vec + index
3. **所有权验证**：编译器保证没有数据竞争（除了无锁队列的 `unsafe`）
4. **对比**：
   - 延迟：Rust 应该跟 C++ 相当（零成本抽象）
   - 安全：Rust 编译期保证无 UB（C++ 靠人肉检查）
   - 开发体验：Rust 编译器帮你找 bug，但学习曲线陡

### 卡住翻哪篇笔记

| 卡住了… | 翻这里 |
|---------|--------|
| Rust 基础 | [Rust ch02](../../18-rust-quant/chapter-02-Rust基础与交易工程搭建.md) |
| Rust 交易引擎 | [Rust ch07](../../18-rust-quant/chapter-07-实盘交易引擎开发.md) |
| unsafe 边界 | [Rust ch02](../../18-rust-quant/chapter-02-Rust基础与交易工程搭建.md) |

---

## 交付物

### Version A：C++ 版

- [ ] 限价订单簿（LOB）数据结构（按价格层级 + 时间优先）
- [ ] 撮合引擎：限价单/市价单、IOC/FOK、成交回报
- [ ] 无锁 SPSC ring buffer（行情入 / 撮合出）
- [ ] 绑核 + `SCHED_FIFO` + 大页 + `mlock`
- [ ] 行情输入：UDP 组播行情解析（复用 P6/P7）
- [ ] 延迟基准：单笔撮合 p50/p99/p999

### Version B：Rust 重写

- [ ] 同功能 Rust 实现（`unsafe` 仅限无锁队列）
- [ ] 所有权/借用验证无数据竞争
- [ ] 对比 C++ 版：延迟、代码安全、开发体验

## 覆盖模块

| 模块 | 用到什么 |
|------|----------|
| [`18` hft-engineering](../../14-hft-engineering/) | LOB、撮合、无锁、绑核、HFT 工程全链 |
| [`21` rust-quant](../../18-rust-quant/) | Rust 所有权、零成本抽象、unsafe 边界 |
| [`22` markets-microstructure](../../19-markets-microstructure/) | Harris：订单类型、撮合规则、queue priority |

## 前置

[P4](../P4-kernel-module/) + [P5](../P5-raspberry-pi-embedded/) + [P7](../P7-dpdk-forwarder-profiling/)（内核/嵌入式/网络性能全过关）。

## 里程碑

1. **M1** LOB 数据结构 + 限价单撮合（单线程正确性）→ Phase 1
2. **M2** 市价单 + IOC/FOK + 成交回报 → Phase 2
3. **M3** 无锁 ring buffer 衔接行情输入 → Phase 3
4. **M4** 绑核/大页/mlock，延迟基准 → Phase 4
5. **M5** perf/bpftrace 剖析，优化尾延迟 → Phase 5
6. **M6** Rust 重写，对比验证 → Phase 6

## 这是你所有学习的收口

| 前面学的 | 在 P8 哪里用 |
|---------|-------------|
| P2 malloc（堆/对齐） | LOB 内存池预分配 |
| P2.5 container_of/链表 | 侵入式链表管理价位队列 |
| P2.5 ring buffer | 行情入队/撮合出队 |
| P2.5 rdtsc | 延迟测量 |
| P3 C++ RAII/模板 | C++ 版资源管理 |
| P4 内核模块 | 理解系统调用开销（为什么要绑核/旁路） |
| P5d 延迟统计 | 直方图 + p99/p999 |
| P6 协议解析 | UDP 组播行情解析 |
| P7 DPDK/perf/bpftrace | 网络收发 + 性能剖析 |

## 状态

⬜ 未开始 → 这是终局项目，前面全部通关后再开始。先从 Phase 1 的 LOB 数据结构开始（纯逻辑，不需要任何特殊环境）。

← [projects 总览](../README.md) · [21 模块](../../14-hft-engineering/) · [22 模块](../../18-rust-quant/)
