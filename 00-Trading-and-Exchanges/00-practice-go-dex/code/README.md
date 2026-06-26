# Go DEX 练手 · 源码

跟视频一致的扁平结构：

```
code/
├── go.mod
├── Makefile
├── main.go             ← 入口，打印 working
├── orderbook.go        ← Order / Limit / Orderbook 三个结构体
└── orderbook_test.go   ← 单测骨架
```

```bash
make build
make run          # 或 go run main.go
make test
```

实践笔记 → [../notes/](../notes/)
