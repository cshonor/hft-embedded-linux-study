# Go DEX 练手 · 源码

独立 **Go module**，与 Harris 理论目录分离。

## 目录（跟视频一致，扁平）

```
code/
├── go.mod       ← Go 模块声明
├── Makefile     ← make build / run / test
├── main.go      ← 程序入口（main 函数）
├── order.go     ← 订单长什么样（M1）
├── book.go      ← 订单簿逻辑（M1）
├── book_test.go ← 单测
└── bin/         ← 编译产物（git 忽略）
```

**没有** `order/`、`book/`、`cmd/` 这些子文件夹 —— 只是同一个项目里分了几个 `.go` 文件，跟视频里「一个 main.go 起步」一样；代码多了再拆文件，不用一开始就建包。

## 常用命令

```bash
make build   # → bin/exchange
make run     # 编译并运行，应打印 working
make test    # 跑 book_test.go
```

## 各文件是干什么的

| 文件 | 含义 | 对应 Harris |
|------|------|-------------|
| `main.go` | 程序从哪开始跑 | — |
| `order.go` | `Order` 结构体：买/卖、限价/市价、价格、数量 | Ch 4 |
| `book.go` | `OrderBook`：挂单、最优买卖价、市价吃单 | Ch 4–5 |
| `match.go` | （M2 再加）撮合成交 | Ch 6 |

实践笔记 → [../notes/](../notes/)
