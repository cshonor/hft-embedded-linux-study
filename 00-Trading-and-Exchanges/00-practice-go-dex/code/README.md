# Go DEX 练手 · 源码

独立 **Go module**，与 Harris 理论目录分离；从 **M1** 起按 [OUTLINE](../OUTLINE.md) 增量提交。

## 初始化（首次在本目录写代码时）

```bash
cd 00-Trading-and-Exchanges/00-practice-go-dex/code
go mod init github.com/<your-org>/practice-go-dex   # 或本地路径
```

## 建议包布局（随里程碑扩展）

```
code/
├── go.mod
├── order/          # M1：Order, Side, Type
├── book/           # M1：OrderBook
├── match/          # M2：Matcher, Trade
├── metrics/        # M3：Spread, Depth
└── cmd/
    └── dexd/       # M4：可选 HTTP 入口
```

## 与理论对照

| 包 | Harris |
|----|--------|
| `order`, `book` | Ch 4–5 |
| `match` | Ch 6 |
| `metrics` | Ch 13–14, 19 |

实践笔记 → [../notes/](../notes/)
