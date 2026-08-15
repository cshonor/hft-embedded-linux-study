# Go DEX 练手 · 源码

跟视频一致的扁平结构：

```
code/
├── go.mod
├── Makefile
├── HARRIS-INDEX.md
├── main.go             ← 演示：MM 后台报价 + 散户市价单成交
├── orderbook.go        ← Order / Limit / Orderbook + AddOrder / BestBid / BestAsk
├── marketmaker.go      ← 极简做市：定时刷新买一 / 卖一（Ch 2 §1）
└── orderbook_test.go
```

```bash
make build
make run          # MM 做市 + 散户市价买卖 demo
make test
```

| 查什么 | 去哪 |
|--------|------|
| 29 章 ↔ 哪段代码 | [HARRIS-INDEX.md](./HARRIS-INDEX.md) |
| 里程碑进度 | [../OUTLINE.md](../OUTLINE.md) |
| 实践笔记 | [../notes/](../notes/) |
| 全书章节目录 | [../OUTLINE.md](../OUTLINE.md)（上级 Harris 目录） |
