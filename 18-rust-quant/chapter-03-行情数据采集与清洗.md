# 第3章 行情数据采集与清洗

> 目标：把网上的字节变成引擎能吃的内部 Event。脏数据不许进 Book。

← [第2章](./chapter-02-Rust基础与交易工程搭建.md) · 下一章：[第4章 K线](./chapter-04-时间序列与K线处理.md)

---

## 管道

```
UDP/TCP/文件  →  切帧  →  解析  →  校验序号  →  归一化 Event  →  无锁环
```

| 步骤 | 要点 |
|------|------|
| 切帧 | 二进制协议带长度；不要 `split('\n')` 当行情 |
| 解析 | `from_le_bytes` / 手写字段；热路径少用 `serde_json` |
| 序号 | 发现 gap → 请求 snapshot，不要拿残缺簿做交易 |
| 归一化 | 各所字段不同，内部只留一种 `Event` |

对应 [14 §3.5 Book Builder](../14-hft-engineering/chapter-03-orderbook-depth-market-data/3.5-本地BookBuilder与行情解析.md) 和 [14 Ch6 协议](../14-hft-engineering/chapter-06-low-latency-network-protocol/README.md)。

P10 demo 没有真实协议，Event 直接在内存里生成——那是教学简化，不是生产。

---

## 清洗（冷一点也可以）

- 价格 / 量 ≤ 0 → 丢弃并计数  
- 时间戳回拨 → 标记、不要当新 tick  
- 重复序号 → 去重  
- 统计丢包率，给 Ch10 监控  

研究用的 CSV 可以在冷路径用 `String`；一旦进引擎，只许 POD 结构（全是整数和 enum）。

---

## Rust 落点

- 读 socket：先同步 `std::net::UdpSocket` 搞懂帧；再考虑 `tokio`（冷路径 / 网关外）。  
- 热路径解析函数签名像 `fn parse(buf: &[u8]) -> Result<Event, ParseErr>`，错误变成「丢包+日志」，不要 panic。

---

## 卡住翻哪篇

| 卡住了… | 翻这里 |
|---------|--------|
| 本地簿必须和撮合语义一致 | [14 §3.2](../14-hft-engineering/chapter-03-orderbook-depth-market-data/3.2-撮合引擎三种场景.md) |
| 字节序 / 切片 | [17 Book Ch4 slices](../17-rust-foundation/00-Book/04-ownership/) |
