# Go DEX 练手 · 源码

跟视频一样的扁平结构，**没有** `order/`、`book/`、`cmd/` 子文件夹。

```
code/
├── go.mod
├── Makefile
├── main.go           ← 入口，打印 working
├── orderbook.go      ← 订单 + 订单簿（跟视频一起写）
├── orderbook_test.go ← 单测（Go 命名：源文件_test.go）
└── bin/              ← make build 产物
```

```bash
make build
make run
make test
```

实践笔记 → [../notes/](../notes/)
