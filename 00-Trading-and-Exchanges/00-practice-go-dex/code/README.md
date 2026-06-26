# Go DEX 练手 · 源码

跟视频一致的扁平结构：

```
code/
├── go.mod
├── Makefile
├── HARRIS-INDEX.md     ← 29 章理论笔记 ↔ 代码对照（查章节用这页）
├── main.go             ← 入口，打印 working
├── orderbook.go        ← Order / Limit / Orderbook 三个结构体
└── orderbook_test.go   ← 单测骨架
```

```bash
make build
make run          # 或 go run main.go
make test
```

| 查什么 | 去哪 |
|--------|------|
| 29 章 ↔ 哪段代码 | [HARRIS-INDEX.md](./HARRIS-INDEX.md) |
| 里程碑进度 | [../OUTLINE.md](../OUTLINE.md) |
| 实践笔记 | [../notes/](../notes/) |
| 全书章节目录 | [../OUTLINE.md](../OUTLINE.md)（上级 Harris 目录） |
