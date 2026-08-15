# 02 · 中低频交易机器人

> 索引：[08 实战项目](../README.md)

## 是什么

**秒～分钟级**量化交易**框架骨架**（非 HFT）：行情 → 策略 trait → 风控 → 订单状态机 → 审计日志。当前为 **可编译 MVP 占位**，逐步接入 SQLite / 交易所 API。

## 架构

```text
MarketFeed (mock / WS) ──► Strategy::on_bar ──► RiskGate ──► Oms ──► AuditLog
                                ▲
                          Config + Clock
```

## 技术栈（规划）

| 模块 | 技术 |
|------|------|
| 运行时 | tokio |
| 策略 | `trait Strategy` 插件 |
| 存储 | SQLite（roadmap） |
| 配置 | TOML + 环境变量 |

## 运行

```bash
cargo run -p midfreq-trading-bot
```

## Roadmap

- [x] 模块划分 + mock 行情一轮
- [ ] `Bar`/`Tick` 归一化 + WebSocket feed
- [ ] 风控：仓位上限、日亏损熔断
- [ ] OMS 状态机 + SQLite 审计
- [ ] 回测与实盘共用 `Strategy` trait

## 设计取舍

1. **明确非微秒 HFT** — 不承诺 co-location 级延迟。
2. **密钥不进仓库** — `.env.example` + 文档说明。
3. **先回测后模拟盘** — 同一策略接口。

## 对外描述（GitHub）

> 中低频量化交易框架：策略插件、风控、OMS 状态机；Rust + tokio，非 HFT。
