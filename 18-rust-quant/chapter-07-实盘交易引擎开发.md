# 第7章 实盘交易引擎开发

> 热路径是一个死循环：弹队列 → 更新簿 → 策略 → 风控 → 下单。不是 web 服务。

← [第6章](./chapter-06-策略回测框架实现.md) · 下一章：[第8章 OMS](./chapter-08-订单管理与路由系统.md)

---

## 主循环

```
loop {
    let ev = ring.pop();          // 没有就空转，不要 sleep
    book.apply(&ev);
    if ev.run_strategy {
        cancel_old_quotes();
        let intents = strategy.quote(&book);
        for o in intents {
            if risk.check(&o) == Ok { book.submit(o); }
        }
    }
}
```

这和 [P10 `engine.hpp`](../projects/P10-hft-prototype/part-a-demo/src/engine.hpp)、本模块 [`engine.rs`](./demo/src/engine.rs) 是同一张图。

Rust 版用所有权保证：**只有引擎线程可变借用 Book**。行情线程（或 replay）只生产 Event。demo 把两步合成单线程 `for e in events { handle(e) }`，语义不变，只是没有排队延迟。

一拍内部顺序不能乱：

1. 先让别人的撤/挂/市价打到簿上（可能打到我们旧单，`on_fill` 改库存）  
2. 再撤我们旧报价  
3. 再按 **新** BBO 挂买卖  
4. 风控不过的单不进 `submit`

---

## 不要把 async 塞进热路径

`tokio` 适合：盘后写库、HTTP 风控面板、重连会话。  
不适合：每个 tick `await`。执行器调度是冷路径预算，不是 T2T 预算。对照 [14 §1.2 热/冷](../14-hft-engineering/chapter-01-hft-fundamentals-ecosystem/1.2-热路径与冷路径.md)。

无锁队列：先理解 [14 §7.2](../14-hft-engineering/chapter-07-lockless-data-structures-memory-layout/7.2-无锁FIFO队列.md)，再决定手写还是 `crossbeam-queue`。`unsafe` 只包在 queue 内部。demo 用 `Vec<Event>` 代替环，就是为了把这一课推迟。

---

## 线程切分（最小）

| 线程 | 职责 | 核 |
|------|------|-----|
| MD | 收包、解析、push 环 | 可与网卡同 NUMA |
| Engine | Book + 策略 + 风控 | isolcpus 独占（生产） |
| Log | 从另一个环 pop 文本 | 随便哪个核 |

demo 和笔记本 WSL **不绑核、不双线程**。生产怎么绑看 [14 Ch5](../14-hft-engineering/chapter-05-os-kernel-tuning/README.md)。

订单号：回放从 1 起，我们的报价从 `1_000_000_000` 起，避免撞号。撤错别人的单，簿会 silently 少档，极难查。

---

## 卡住翻哪篇

| 卡住了… | 翻这里 |
|---------|--------|
| C++ 引擎清单 | [14 Ch8](../14-hft-engineering/chapter-08-ultra-low-latency-engine-dev/README.md) |
| 原子与内存序 | [17 atomic](../17-rust-foundation/05-Async-Concurrency-Network/01-atomic/) |
| 本模块引擎 | [`demo/src/engine.rs`](./demo/src/engine.rs) |
