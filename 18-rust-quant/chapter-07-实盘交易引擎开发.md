# 第7章 实盘交易引擎开发

> 热路径是一个死循环：弹队列 → 更新簿 → 策略 → 风控 → 下单。不是 web 服务。

← [第6章](./chapter-06-策略回测框架实现.md) · 下一章：[第8章 OMS](./chapter-08-订单管理与路由系统.md)

---

## 主循环

```
loop {
    let ev = ring.pop();          // 没有就空转，不要 sleep
    book.apply(&ev);
    if ev.should_quote {
        let intents = strategy.on_book(&book);
        for o in intents {
            if risk.allow(&o) { book.submit(o); }
        }
    }
}
```

这和 [P10 `engine.hpp`](../projects/P10-hft-prototype/part-a-demo/src/engine.hpp) 是同一张图。Rust 版用所有权保证：**只有引擎线程可变借用 Book**，行情线程只往环里 `push`。

---

## 不要把 async 塞进热路径

`tokio` 适合：盘后写库、HTTP 风控面板、重连会话。  
不适合：每个 tick `await`。执行器调度是冷路径预算，不是 T2T 预算。对照 [14 §1.2 热/冷](../14-hft-engineering/chapter-01-hft-fundamentals-ecosystem/1.2-热路径与冷路径.md)。

无锁队列：先理解 [14 §7.2](../14-hft-engineering/chapter-07-lockless-data-structures-memory-layout/7.2-无锁FIFO队列.md)，再决定手写还是 `crossbeam-queue`。`unsafe` 只包在 queue 内部。

---

## 线程切分（最小）

| 线程 | 职责 | 核 |
|------|------|-----|
| MD | 收包、解析、push 环 | 可与网卡同 NUMA |
| Engine | Book + 策略 + 风控 | isolcpus 独占（生产） |
| Log | 从另一个环 pop 文本 | 随便哪个核 |

demo 和笔记本 WSL **不绑核**。生产怎么绑看 [14 Ch5](../14-hft-engineering/chapter-05-os-kernel-tuning/README.md)。

---

## 卡住翻哪篇

| 卡住了… | 翻这里 |
|---------|--------|
| C++ 引擎清单 | [14 Ch8](../14-hft-engineering/chapter-08-ultra-low-latency-engine-dev/README.md) |
| 原子与内存序 | [17 atomic](../17-rust-foundation/05-Async-Concurrency-Network/01-atomic/) |
| 本模块订单簿 | [`demo/`](./demo/) |
