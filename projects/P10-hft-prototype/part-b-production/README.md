# P10 part-b — 生产形态 HFT 引擎

> part-a 是 1348 行的全链路 demo；part-b 按**生产工程标准**重写：
> 分模块、可单测、错误走返回值、热路径零分配零异常，目标平台 **Linux（Pi 5 实测）**。
>
> **诚实的边界**：这是"软件生产形态"，不是"可上实盘"。缺的三样是物理性的——
> colo 机房、交易所认证连接、硬件 PTP 对时。引擎本身（协议/恢复/风控/延迟纪律）与生产同构。

## 六阶段路线图

| Phase | 内容 | 状态 |
|-------|------|------|
| 1 | 工程骨架 + 线协议（messages/codec/sequencer）+ 自测 | ✅ 完成（93 断言全过） |
| 2 | 订单簿/撮合生产化：内存池 + 侵入式价位队列 + O(1) 撤改 | ✅ 完成（99 断言全过） |
| 3 | UDP 组播收发 + 乱序缓冲 + gap 重传恢复；回放=实盘同码路径 | ⬜ |
| 4 | 策略接口 + 本地拒单链风控（fat finger/仓位/频率/领口）+ kill switch | ⬜ |
| 5 | Pi 5 绑核/SCHED_FIFO/mlock/大页 + p50/p99/p999 延迟基准 | ⬜ |
| 6 | lock-free 异步审计日志 + 架构文档 + benchmark 报告 | ⬜ |

## 构建与自测

```bash
# Mac（仅编译期验证，延迟数字无意义）
cmake -S . -B build && cmake --build build && ./build/test_protocol

# Pi 5（生产目标，-march=native 打开本机优化）
cmake -S . -B build -DCMAKE_BUILD_TYPE=Release -DHFT_NATIVE=ON
cmake --build build -j4 && ctest --test-dir build
```

编译纪律：`-Wall -Wextra -Wpedantic -Werror -fno-exceptions -fno-rtti`，Release 默认 `-O3`。

## 目录

```
src/
├── common/    types（定点价格）· time（rdcycle/now_ns 双时钟）· compiler（分支预测/对齐宏）
├── protocol/  messages（ITCH 风格消息）· codec（大端编解码）· sequencer（gap 状态机）
├── md/        [Phase 3] 组播行情接入
├── book/      memory_pool（slab 池）· level_bitmap（两级位图）· order_book（撮合）
├── strategy/  [Phase 4] 策略接口
├── risk/      [Phase 4] 拒单链
├── gateway/   [Phase 3+] 订单网关（模拟撮合端）
└── infra/     [Phase 5/6] 绑核/大页/审计日志
tests/         自测（无第三方依赖，断言宏 30 行）
```

## 协议速览（Phase 1 定稿）

帧：`[u16 总长][u8 类型][u64 seq][u64 ts][payload]`，大端，定长，一个 UDP 报文可装多条。

| 类型 | 字节 | 用途 |
|------|------|------|
| `H` Heartbeat | 0 | 无行情时段保活 |
| `S`/`E` SessionStart/End | 4/0 | 交易日边界，重置序列状态 |
| `A` NewOrder | 29 | 挂单（book 增量） |
| `X` CancelOrder | 8 | 撤单 |
| `R` ReplaceOrder | 24 | 改单价/量 |
| `T` Trade | 32 | 成交（含吃单方/挂单方双 id） |
| `G` GapFill | 16 | 重传端告知"该区间无需恢复"，直接跳号 |

设计取舍（与 demo 的本质区别）：**不**用 packed struct 强转（未对齐 UB + 耦合内存布局），
解码一次性转成宿主机序原生结构体，热路径只碰原生类型；畸形报文只会产生 reject，不可能 UB。

## 序列号恢复语义（生产纪律）

- `seq == expect` → 交付并推进；`seq < expect` → 重传/重复，静默丢弃；`seq > expect` → gap，触发恢复；
- gap 期间**不推进 expect、不更新 book**（宁可停一手，不做错一手）；
- `GapFill` 必须正好从 expect 开始，否则判定对端状态不一致（生产要告警/断流）。

## 订单簿速览（Phase 2 定稿）

| 结构 | 选型 | 为什么 |
|------|------|--------|
| 价位索引 | 数组 + 两级位图（非红黑树） | tick 域有界（价格带），O(1) 定位、无指针追逐；最优价缓存 + 打空时位图重扫 |
| 价位内队列 | 侵入式双向链表 | FIFO = price-time priority，O(1) 入出队 |
| 订单定位 | id → Order\* 链地址哈希（负载 ≤ 0.5） | O(1) 撤/改 |
| 内存 | 固定容量 slab 池 | init 一次性分配，热路径零堆分配；耗尽返回 `PoolExhausted` 拒单 |

撮合语义：成交价 = 挂单方价（价格改善给吃单方）；市价单余量 IOC 不驻留；
同价减量改单保留排队位置，改价/加量 = 撤旧挂新丢位置（交易所通行规则）；
自成交防护（STP）放在风控层，簿保持纯粹。
