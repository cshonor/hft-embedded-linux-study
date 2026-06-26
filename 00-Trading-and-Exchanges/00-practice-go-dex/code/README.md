# Go DEX 练手 · 源码

独立 **Go module**，与 Harris 理论目录分离；从 **M1** 起按 [OUTLINE](../OUTLINE.md) 增量提交。

## 常用命令

在 `code/` 目录下：

```bash
make        # 默认跑 test
make build  # 编译全部包
make test   # go test -v ./...
```

## 包布局（随里程碑扩展）

```
code/
├── go.mod
├── Makefile
├── order/          # M1：Order, Side, Type
├── book/           # M1：OrderBook, BestBid/Ask, TakeMarket
├── match/          # M2：Matcher, Trade（待建）
├── metrics/        # M3：Spread, Depth（待建）
└── cmd/
    └── dexd/       # M4：可选 HTTP 入口（待建）
```

## 与理论对照

| 包 | Harris | 状态 |
|----|--------|------|
| `order`, `book` | Ch 4–5 | M1 ✅ |
| `match` | Ch 6 | M2 |
| `metrics` | Ch 13–14, 19 | M3 |

实践笔记 → [../notes/](../notes/)
