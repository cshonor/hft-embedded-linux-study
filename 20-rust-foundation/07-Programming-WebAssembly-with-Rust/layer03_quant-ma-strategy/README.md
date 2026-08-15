# Layer 3 · 量化场景：轻量均线策略 Wasm

> 上层索引：[三层学习架构](../三层学习架构.md)  
> 原书：**第 5 章** [Yew](../chapter05_yew/README.md) · **第 6～8 章** [非 Web 宿主 / IoT / WARoS](../chapter06_nonweb_hosts/README.md) · [附录](../appendix/README.md)

**目标**：用 Rust 写 **轻量均线（MA）策略** 核心，编译为 Wasm：

1. **浏览器**：Yew 面板拉历史/模拟 tick，Wasm 内跑回测，画权益曲线。  
2. **实盘/边缘**：同一 `.wasm` 由 **wasmtime / wasmer**（或 Rust 宿主，原书 Ch6）加载，接入 Layer 2 订单簿 mid → 信号 → 下游执行（仅 scaffold，不下真单）。

---

## 核心子主题（原书对应）

| 原书 | 子主题 | 本层映射 |
|------|--------|----------|
| **Ch5** | Yew 入门 · **实时聊天** | Yew **回测仪表盘**（替换聊天为 K 线/MA/仓位） |
| **Ch6** | 合格宿主 · Rust 解释 Wasm · **控制台**宿主 | **实盘宿主**：加载 `ma_strategy.wasm`，喂 tick |
| **Ch7** | 指示器模块 · **ARM** · **树莓派** | 边缘节点跑同一 Wasm 策略 |
| **Ch8** | WARoS API · **比赛引擎** · Wasm 机器人 · 云端 | 策略模块 = 机器人；引擎 = tick 调度/沙箱 |
| **A1** Serverless | OpenFaaS · 云端 Wasm | 可选：策略函数冷启动评估 |
| **A2** 安全 | 签名 · 加密 | **实盘必做**：Wasm 模块签名与导入能力白名单 |

---

## 策略规格（最小可行）

```rust
// 概念 API（最终实现放 demo/ma_core/src/lib.rs）
pub struct MaState { period: u32, sum: f64, queue: ... }
pub fn on_tick(state: &mut MaState, price: f64) -> MaSignal { ... }
pub enum MaSignal { Flat, Long, Short }
```

| 模式 | 宿主 | 输入 | 输出 |
|------|------|------|------|
| **回测** | 浏览器 + Yew | 历史 CSV / 模拟 tick 流 | 权益曲线、交易列表 |
| **纸面/边缘** | wasmtime CLI / Rust 宿主 | Layer 2 **mid price** 流 | 信号 enum（不直接下单） |

**HFT 边界（笔记必写）**：

- Wasm **不适合** sub-μs 撮合热路径；本层练 **策略沙箱化 + 一套代码多宿主**，热路径仍 native（见 [05-atomic](../../05-Async-Concurrency-Network/01-atomic/README-学习区.md)）。
- 实盘嵌入时：策略 **无网络导入**；行情由宿主 **memory / host call** 注入。

---

## 架构

```text
                    ┌─────────────────────────────────┐
                    │  ma_core.wasm (Rust, no_std 可选) │
                    │  on_tick · MaState · MaSignal      │
                    └───────────────┬─────────────────┘
            ┌───────────────────────┼───────────────────────┐
            ▼                       ▼                       ▼
   ┌─────────────────┐    ┌─────────────────┐    ┌─────────────────┐
   │ Yew 回测 UI      │    │ wasmtime 宿主    │    │ 边缘节点 (Ch7)   │
   │ (Ch5)           │    │ (Ch6)           │    │ ARM / Pi        │
   │ 历史 tick 回放   │    │ Layer2 mid 流   │    │ 同 wasm 二进制   │
   └─────────────────┘    └─────────────────┘    └─────────────────┘
```

对照 [8.3 match-engine](../chapter08_waros/8.3-match-engine.md)：引擎按 tick 调用策略导出函数，而非策略内线程轮询。

---

## 练手 demo（规划）

```text
layer03_quant-ma-strategy/
├── README.md
├── demo/
│   ├── ma_core/              ← 策略内核 → cdylib wasm + rlib 供测试
│   ├── backtest_yew/         ← Trunk + Yew 回测页（改 Ch5 聊天结构）
│   ├── host_wasmtime/        ← Rust 二进制：读 stdin tick，调 wasm
│   └── fixtures/
│       └── sample_ticks.csv
└── notes/
    ├── 回测-vs-实盘-边界.md
    └── A2-模块签名清单.md
```

| 里程碑 | 验收 | 状态 |
|--------|------|:----:|
| M1 | `ma_core` 单元测试：已知 tick 序列 → MA 交叉信号 | 📄 |
| M2 | Yew 页：加载 wasm，跑 CSV，显示简单权益曲线 | 📄 |
| M3 | wasmtime 宿主：stdin 喂价，stdout 打信号 | 📄 |
| M4 | 附录 A2：模块 hash / 签名流程写进笔记 | 📄 |

---

## 前置

- [Layer 2 · 订单簿查询](../layer02_orderbook-query-wasm/README.md) — mid price 来源  
- [Layer 1 · 编译链路](../layer01_rust-llvm-to-wasm/README.md) — 同一 `on_tick` 导出 `.ll` 对比  
- [chapter08_waros/8.3](../chapter08_waros/8.3-match-engine.md) — 调度模型  
- [appendix/A2](../appendix/A2-security.md) — 实盘安全  

---

## 速记

```text
同一 ma_core.wasm：浏览器回测(Yew) ║ 实盘宿主(wasmtime) ║ 边缘(Ch7)
原书聊天 → 回测 UI；原书 WARoS 引擎 → tick 调度；Appendix A2 → 签名
```
