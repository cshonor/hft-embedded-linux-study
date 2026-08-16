# 5.4 结构体持久化 demo — JSON 落盘

笔记：[5.4-结构体与持久化.md](../5.4-结构体与持久化.md)

## 结构

```
src/
├── model/trade.rs   # Trade struct + Serialize/Deserialize
└── main.rs          # 模拟 db：写 trade.json、读回
```

## 运行

```bash
cargo run
```

会在当前目录生成 `trade.json`（已在 `.gitignore` 可选手动删）。

Serde 基础见 [3.2-json-encoding-demo](../../03-common-concepts/3.2-json-encoding-demo/)。
