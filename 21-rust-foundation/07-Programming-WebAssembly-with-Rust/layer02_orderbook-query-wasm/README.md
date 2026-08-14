# Layer 2 · 订单簿查询 Wasm 模块

> 上层索引：[三层学习架构](../三层学习架构.md)  
> 原书：**第 2 章** [跳棋/数据结构](../chapter02_wasm_checkers/README.md) · **第 3～4 章** [Rust Wasm + JS 集成](../chapter03_rust_wasm/README.md)

**目标**：写一个 **Wasm 模块**，在浏览器或轻量宿主中调用你已有的 **Go 订单簿服务** 查询接口，解析快照并在 Wasm 线性内存里做 **best bid/ask、档位深度** 等只读查询 — 把原书 Checkers「紧凑棋盘状态」换成 **订单簿档位布局**。

---

## 核心子主题（原书对应）

| 原书 | 子主题 | 本层映射 |
|------|--------|----------|
| **Ch2** | 数据结构限制 · 规则 · 走子 · **测试 Wasm** | 档位 `#[repr(C)]` · 只读查询逻辑 · `wasm-bindgen-test` |
| **Ch3** | Hello Wasm · Rust 引擎 · **Wasm 接口** · JS 游玩 | 导出 `query_top_of_book` · `fetch` Go API |
| **Ch4** | **wasm-bindgen** · Rogue · 深入实验 | bindgen 封 HTTP 响应 · 错误码进线性内存 |

笔记 → [chapter02_wasm_checkers/](../chapter02_wasm_checkers/README.md) · [chapter03_rust_wasm/](../chapter03_rust_wasm/README.md) · [chapter04_js_integration/](../chapter04_js_integration/README.md)

---

## 外部依赖：Go 订单簿服务

> **不在本 Rust 仓** — 对接你之前实现的 Go 订单簿查询服务（HTTP 或 gRPC，按你实际接口填）。

### 假定接口形状（占位，按你的 Go 服务改）

```text
GET /api/v1/book?symbol=BTCUSDT
→ JSON: { "bids": [[price, qty], ...], "asks": [...], "ts": 1234567890 }
```

| 字段 | Wasm 侧用途 |
|------|-------------|
| `bids[0]` | best bid — Layer 3 均线/信号输入 |
| `asks[0]` | best ask |
| 多档 | 深度不平衡、滑点估算（可选） |

在笔记里记录：**Go 服务仓库 URL**、端口、鉴权方式。

---

## 架构

```text
┌─────────────────┐     fetch      ┌──────────────────┐
│  JS / 轻量页面   │ ──────────────→│  Go 订单簿 API    │
└────────┬────────┘                └──────────────────┘
         │ wasm-bindgen
         ▼
┌─────────────────┐
│  orderbook_wasm │  parse JSON → 线性内存 BookSnapshot
│  (Rust → Wasm)  │  export: best_bid(), best_ask(), spread()
└─────────────────┘
```

**设计要点**（对齐原书 2.2 + 3.5）：

- 快照用 **固定上限档位数**（如 20 档），避免 Wasm 里动态扩容踩坑。
- 价格/数量用 **`u64` 定点** 或 `f64`（与 Go 侧一致），见 [ER Item 01 基本类型](../../01-ER/Chapter-01-Types/Item-01-express-data-structures/01-fundamental-types.md)。
- **网络在 JS 宿主**；Wasm 只做 **解析 + 查询** — 便于同一模块 later 嵌入非 Web 宿主（Layer 3）。

---

## 练手 demo（规划）

```text
layer02_orderbook-query-wasm/
├── README.md                 ← 本文件
├── demo/
│   ├── orderbook_wasm/       ← cdylib + wasm-bindgen
│   │   ├── Cargo.toml
│   │   └── src/lib.rs        ← BookSnapshot, best_bid, spread
│   └── web/                  ← 最小 HTML + wasm-pack 输出
│       └── index.html
└── notes/
    └── Go-API-对接笔记.md    ← 你的 Go 服务实际 URL/字段
```

| 里程碑 | 验收 | 状态 |
|--------|------|:----:|
| M1 | `wasm-pack build` 成功 | 📄 |
| M2 | 浏览器拉 Go API，页面显示 best bid/ask | 📄 |
| M3 | 单元测试：固定 JSON → spread 正确 | 📄 |

---

## 与 Layer 1 / 06 的衔接

- 用 Layer 1 流程对 `lib.rs` 再导出一份 **`.ll`**，对比「解析循环」在 native IR vs Wasm 的差异。
- Checkers [2.2 位标志](../chapter02_wasm_checkers/2.2-data-structure-constraints.md) 笔记里加一节：**档位 occupancy 位图**（可选优化）。

---

## 下一层

→ [Layer 3 · 均线策略](../layer03_quant-ma-strategy/README.md)（消费 Layer 2 的 mid price · Yew 回测 · 实盘嵌入）
