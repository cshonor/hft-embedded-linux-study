# P10 part-a — Demo 级 HFT 全链路

> 一篇设计说明 → [../docs/design.md](../docs/design.md)

把笔记里的关键路径收成**可编译、可测、可看 PnL** 的单机程序：

```
行情回放 --SPSC--> 订单簿 -> 做市策略 -> 本地风控 -> 模拟撮合 -> PnL / p50·p99·p999
```

**不是实盘。** 没有 DPDK、没有共址、没有真实交易所。

---

## 构建

需要 C++17。本仓库 Windows 侧用 WSL 的 g++：

```bash
cd projects/P10-hft-prototype/part-a-demo
make
./hft_demo --self-test
./hft_demo
./hft_demo --ticks 50000 --seed 1
```

或 CMake：

```bash
cmake -S . -B build -DCMAKE_BUILD_TYPE=Release
cmake --build build
./build/hft_demo --self-test
```

## 参数

| 参数 | 含义 | 默认 |
|------|------|------|
| `--ticks N` | 回放 tick 数 | 20000 |
| `--seed N` | 随机种子 | 1 |
| `--hits P` | 主动单概率 | 0.35 |
| `--jump N` | 每 N tick 一次跳价（0=关闭） | 80 |

跳价用来演示逆向选择：旧报价被捡走，库存和 PnL 会变差。`--jump 0` 再跑一次，对比很明显。

## 源码

| 文件 | 做什么 |
|------|--------|
| `src/types.hpp` | tick 价格、订单、事件 |
| `src/spsc_ring.hpp` | 无锁 SPSC 环 |
| `src/orderbook.hpp` | LOB + FIFO 撮合 |
| `src/strategy.hpp` | 库存偏斜做市 |
| `src/risk.hpp` | 价格带 / 仓位 / 流速 |
| `src/replay.hpp` | 随机游走行情 |
| `src/engine.hpp` | 串起来 |
| `src/self_test.hpp` | 撮合与风控断言 |
| `src/main.cpp` | 入口 |

卡住翻哪篇 → [docs/design.md §2](../docs/design.md)
