# P10 Demo 设计：单机 HFT 全链路

> 一篇文讲清：这条 demo 在仿什么、故意没仿什么、每一段代码对应哪章笔记。  
> 可运行工程 → [`../part-a-demo/`](../part-a-demo/)

---

## 先给结论

这不是实盘系统，也不是 DPDK / 共址 / FPGA。  
它是一条**能编译、能跑、能看数字**的教学链路：

```
行情模拟器 ──SPSC 环──► 本地订单簿 ──► 做市策略 ──► 本地风控 ──► 模拟撮合 ──► PnL + 延迟分位
```

对应笔记里的关键路径（[14-hft-engineering 1.1](../../../14-hft-engineering/chapter-01-hft-fundamentals-ecosystem/1.1-系统核心架构.md)）：

```
Exchange ──► Gateway IN ──► Book Builder ──► Strategy ──► OMS ──► Gateway OUT ──► Exchange
```

demo 把 **Exchange + Gateway** 收成「同进程模拟交易所」，把 **OMS 风控**收成热路径上的几次 `if`。  
价值是把 `14-hft-engineering` 的骨架跑通，而不是把延迟打到纳秒。

---

## 1. 为什么先写 demo，而不是直接上 P8/P10 终局

| | 终局 P10（原 README） | 本 demo（part-a） |
|--|----------------------|-------------------|
| 行情 | DPDK 收 UDP 组播 | 内存里生成随机游走 + 主动单 |
| 订单簿 | 红黑树 + 哈希 + 内存池 | `std::map` + `std::list`（正确性优先） |
| 进程模型 | 多进程绑核 | 两线程 + SPSC |
| 下单 | 模拟成交（仍偏工程） | 同一本簿上撮合，我们是其中一个 owner |
| 环境 | Linux + hugepage + isolcpus | Windows / WSL 一条 `make` |
| 目标 | 求职级原型 | **先看见全链路动起来** |

P8 的 Phase 1 骨架（限价簿 + 价格优先/时间优先）被直接做成可测代码。  
DPDK、绑核、大页、火焰图仍按 [P10 README](../README.md) 的 M9–M12 往后排。

---

## 2. 系统切成六块

```
┌────────────┐     Event      ┌──────────────────────────────────────────┐
│  Replay    │ ─────────────► │              Engine 线程                  │
│  生产者线程 │    SPSC ring   │                                          │
└────────────┘                │  1. 把外来单送进 OrderBook（模拟交易所）     │
                              │  2. 成交回报 → 更新我方仓位                 │
                              │  3. 策略看 BBO / 库存 → 生成报价            │
                              │  4. 风控：价格带 / 量 / 仓位 / 流速 / kill │
                              │  5. 通过才下单；拒绝走本地，不出「交易所」    │
                              │  6. 记 tick→decision 延迟直方图            │
                              └──────────────────────────────────────────┘
```

| 模块 | 文件 | 笔记 |
|------|------|------|
| 类型（整数价格、订单、成交） | `types.hpp` | Ch3：价格用 tick，不用 `double` |
| SPSC 无锁环 | `spsc_ring.hpp` | [Ch7.2 无锁 FIFO](../../../14-hft-engineering/chapter-07-lockless-data-structures-memory-layout/7.2-无锁FIFO队列.md) |
| 订单簿 + 撮合 | `orderbook.hpp` | [Ch3.2 三种场景](../../../14-hft-engineering/chapter-03-orderbook-depth-market-data/3.2-撮合引擎三种场景.md) · [Ch3.3 FIFO](../../../14-hft-engineering/chapter-03-orderbook-depth-market-data/3.3-同价匹配算法.md) |
| 做市策略 | `strategy.hpp` | [Ch12.1 价差−逆向选择−库存](../../../14-hft-engineering/chapter-12-market-making-arbitrage/12.1-做市核心等式与逆向选择.md) |
| 本地风控 | `risk.hpp` | [Ch10.1 拒单链](../../../14-hft-engineering/chapter-10-risk-compliance-slippage/10.1-本地拒单链.md) |
| 回放 + 引擎 + 延迟 | `replay.hpp` `engine.hpp` `latency.hpp` | [Ch1.1 架构](../../../14-hft-engineering/chapter-01-hft-fundamentals-ecosystem/1.1-系统核心架构.md) · [Ch9 测量](../../../14-hft-engineering/chapter-09-latency-measurement-benchmarking/README.md) |

---

## 3. 撮合语义（必须和笔记一致）

输入：新订单 + 当前簿。输出：成交列表，以及限价单的剩余挂单。

| 场景 | demo 行为 | 笔记例子 |
|------|-----------|----------|
| **Best Price** | 买单先吃最低卖；成交价 = **挂单价**（价格改善归 taker） | 买 @$100，簿上有 $99 → 先 $99 |
| **Partial Fill** | 吃完一档继续下一档，限价剩余再 rest | 买 4，最优卖 1 → 先成交 1 |
| **No Match** | 买价 < 最低卖 → 整单挂上 | 买 @$98 vs 卖 $99 |
| **FIFO** | 同价 `list` 队头先成交；cancel/replace = 撤掉重挂 = **去队尾** | Ch3.3 |
| **STP** | 同一 `owner` 的单互不成交，避免自成交 | Ch10.1 |

订单类型：Limit / Market / IOC / FOK。策略走 Limit 双边报价；回放里的冲击单走 Market。

价格一律 `int64` tick。demo 里 **1 tick = 0.01**，起始公允价 `10000`（显示成 100.00）。

---

## 4. 做市怎么赚钱、怎么亏

核心等式（Ch12.1）：

```
PnL = 成交量 × 价差收入  − 逆向选择  − 库存风险
```

demo 策略：

1. 看中间价 `mid = (best_bid + best_ask) / 2`
2. 基础半价差（默认 2 tick）+ 波动加宽
3. **库存偏斜**：库存 > 0 则买价下移、卖价下移（更想卖掉）
4. 每 tick 先撤旧报价，再挂新买卖（教学用 cancel-replace；FIFO 下这会丢队首，是故意暴露的代价）

回放侧会：

- 放一个**更宽**的「别人做市」盘口，保证簿上始终有 BBO
- 随机打入主动买卖，先撞**更窄**的我方报价 → 我们吃到价差
- 偶尔来一次 **跳价**（公允价突然挪 5–8 tick）→ 旧报价被捡走 → 逆向选择，PnL 变差

跑完看三件事：成交次数、期末库存、标记 PnL。  
价差赚得多、跳价少 → PnL 为正；跳价密、库存堆起来 → 很容易转负。这就是笔记里那句话：

> 最愿意让你成交的时刻，往往是你最不该成交的时刻。

---

## 5. 风控：策略 bug 也出不了门

热路径只做独立于策略的硬检查（Ch10.1）：

| 检查 | 防什么 |
|------|--------|
| 价格带（相对 mid） | 乌龙指打穿盘口 |
| 单笔数量上限 | 巨量单 |
| 持仓上限 | 库存滚雪球；减仓单仍放行 |
| 每 tick 下单次数 | 流速 |
| kill switch | 人工/异常停机 |

拒绝发生在本地，**不会**把单子送进撮合。这就是 OMS「违规单不出 Gateway OUT」。

demo 不追求 < 200ns，只保证：无系统调用、无锁、无 `malloc`（检查本身不分配）。

---

## 6. 延迟测什么

生产测的是 **T2T**（交易所 NIC 入 → 我方 NIC 出），要 PTP / 硬件时间戳。  
demo 测的是 **进程内**：SPSC 弹出事件 → 策略+风控+下单 结束。

用 `std::chrono::steady_clock` 记两份直方图：

- **compute**：弹出之后，策略 + 风控 + 下单
- **queue**：事件在 SPSC 里等消费者

笔记本 / WSL 上 queue 往往比 compute 大一个数量级——没绑核、生产者比消费者快时就会堆队列。这本身就是 Ch5 要讲的：隔离和背压没做，尾延迟就不稳。数字用来证明「有分位」，不能拿去和实盘 T2T 比。

---

## 7. 数据流（一个 tick）

```
Replay:
  fair += walk
  撤掉并重挂「别人」的宽买卖
  有时再丢一笔 Market 冲击
  Event.t0 = now()
  ring.push(Event)

Engine:
  pop Event
  book.submit(外来单)           → 可能打到我方挂单
  strategy.on_fill(...)         → 改库存 / cash
  intents = strategy.quote(book)
  for o in intents:
      if !risk.allow(o): reject
      else book.submit(o)
  latency.add(now() - Event.t0)
```

两个 `owner`：

- `0` = 市场（回放）
- `1` = 我方策略

同一本 `OrderBook` 既是「交易所」，也是策略看到的本地簿。demo 里没有单独的 Book Builder 镜像——这是和实盘最大的简化：实盘本地簿由行情 feed 重建，交易所簿在对面机房。

---

## 8. 怎么跑、怎么读输出

```bash
cd projects/P10-hft-prototype/part-a-demo
make
./hft_demo --self-test    # 撮合/风控正确性
./hft_demo                # 默认 20000 tick
./hft_demo --ticks 50000 --seed 1
```

Windows 没有 g++ 时，用 WSL：

```bash
wsl -d Ubuntu -- bash -lc 'cd /mnt/c/Users/12392/Desktop/hft/projects/P10-hft-prototype/part-a-demo && make && ./hft_demo --self-test && ./hft_demo'
```

报告里优先看：

1. `self-test` 全过 —— 语义没写错
2. `our fills` / `PnL` —— 做市等式是否在工作
3. `risk rejects` —— 风控是否真的挡过单
4. `p50/p99/p999` —— 有分位，不只是平均值

---

## 9. 明确不做的事（边界）

| 不做 | 原因 |
|------|------|
| DPDK / OpenOnload / FPGA | 那是 P7 和终局 P10；Windows 笔记本也跑不了 |
| 真实交易所协议（ITCH/OUCH） | demo 事件就是内部 struct |
| hugepage / isolcpus / SCHED_FIFO | POSIX 调优，放到 P8 Phase 4 |
| 双机热备、审计、合规报送 | 生产问题 |
| 用 `double` 当价格 | 笔记里明确禁止 |

**这个 demo 的价值：学习全链路 + 对着笔记指代码。不能上实盘。**

---

## 10. 下一步（仍按原 P10 路线）

1. 把 `std::map` 换成价格网格数组（固定 tick 范围，O(1) 摸到 BBO）
2. 价位队列改侵入式链表 + 对象池（P2.5）
3. 行情改为 UDP 回放，再接到 P7 DPDK
4. 绑核 + 延迟报告进 `docs/benchmark.md`

卡住翻：[P8 迷你撮合](../../P8-matching-engine/README.md) · [14-hft-engineering](../../../14-hft-engineering/README.md) · [1.8 实战启动](../../../14-hft-engineering/chapter-01-hft-fundamentals-ecosystem/1.8-实战启动建议.md)
