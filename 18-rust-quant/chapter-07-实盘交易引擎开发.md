# 第7章 实盘交易引擎开发

> 热路径是一个死循环：弹队列 → 更新簿 → 策略 → 风控 → 下单。不是 web 服务。

← [第6章](./chapter-06-策略回测框架实现.md) · 下一章：[第8章 OMS](./chapter-08-订单管理与路由系统.md)

---

## 本节讲什么

引擎的全部复杂度集中在「一拍的内部顺序」。本章逐段拆 [`demo/src/engine.rs`](./demo/src/engine.rs) 的 `handle()`（:104-153），讲清三件事：为什么六个步骤的顺序一步都不能乱、Rust 的所有权怎么替换了 C++ 里靠纪律维持的线程约定、以及 `LatencyHist` 的 p50/p99 是怎么算出来又该怎么读的。

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

Rust 版用所有权保证：**只有引擎线程可变借用 Book**。行情线程（或 replay）只生产 `Event`。demo 把两步合成单线程 `for e in events { handle(e) }`（`engine.rs:235-240`），语义不变，只是没有排队延迟。

### 一拍内部的六步（顺序不能乱）

`handle()`（`engine.rs:104-153`）的完整顺序：

```
① 事件入账：events += 1
② 市场动作落到簿上：Cancel → book.cancel()；Order → book.submit() + apply_trades()
   ↑ 这一步可能打到我们的旧挂单，on_fill 先改库存
③ 边界检查：!run_strategy 就 return（只是一笔市场更新）
④ 拍开始：ticks += 1；risk.on_tick_start() 重置本拍流速计数；t0 = Instant::now()
⑤ 撤旧报价：live_bid_id / live_ask_id 非 0 则 cancel 并清零
⑥ 报价：mm.quote() 出意图 → 逐张过 risk.check() → 过了才 book.submit()
   → 成交回报再走 on_fill → 记住新的 live_*_id
收尾：latency.add(t0.elapsed())
```

两个顺序约束是这个设计的命脉：

- **② 必须先于 ⑤⑥**：别人这拍的市价冲击可能已经把我们的旧卖单打掉了。如果先挂新单再入账冲击，`on_fill` 改库存的时机就错了——策略会按旧库存算 skew。
- **⑤ 必须先于 ⑥**：先撤旧再挂新，簿上任何时刻我们最多各有一张买卖在途。反过来做会出现一拍两张买单的窗口，仓位上限的判断就失真。

`apply_trades()`（`engine.rs:98-102`）把成交回报喂回策略，是②和⑥共用的收口——不管成交来自别人的冲击还是我们自己的新单，库存和现金的更新走同一条路。

### id 命名空间

回放的事件 id 从 1 起，我们的报价从 `1_000_000_000` 起（`engine.rs:92` 的注释写明了动机：「和回放 id（从 1 起）错开，避免撞号」）。撤错别人的单，簿会 silently 少档，极难查。

---

## 延迟测量：p50/p99 怎么来的

`LatencyHist`（`engine.rs:30-67`）是 30 行的最小延迟直方图：

```rust
fn add(&mut self, ns: u64) {
    self.samples.push(ns);   // 热路径里只做一次 Vec::push
}

fn percentile(&self, p: f64) -> u64 {
    let mut v = self.samples.clone();   // 排序是冷路径：报表时才做
    v.sort_unstable();
    let idx = ((p * (v.len() as f64 - 1.0)).round() as usize).min(v.len() - 1);
    v[idx]
}
```

设计上有两个刻意的取舍：

| 取舍 | 做法 | 理由 |
|------|------|------|
| 采样点不排序 | `add` 只 `push` | 排序 O(n log n) 不能进热路径 |
| 容量预分配 | `Vec::with_capacity(20_000)` | 热路径里不触发 realloc |
| 报表才计算 | `percentile` 里 clone + sort | 冷路径随便花 |

量的对象是 `t0.elapsed()`（`engine.rs:123` → `:152`），即 **quote + risk + submit** 这一段纯计算——不含网络、不含排队。报表里印的「本 demo 单线程，没有 SPSC 排队延迟；对照 P10 的 queue_wait」（`engine.rs:209`）就是在提醒：这个 p99 只是 T2T 里最小的一段，别当成系统延迟。

读 p99 的纪律：p50 告诉你典型有多快，p99 告诉你**尾部有多坏**。HFT 按 p99 设计容量，不按 p50——均值和 p50 在毛刺面前都会撒谎。

---

## 不要把 async 塞进热路径

`tokio` 适合：盘后写库、HTTP 风控面板、重连会话。  
不适合：每个 tick `await`。执行器调度是冷路径预算，不是 T2T 预算。对照 [14 §1.2 热/冷](../14-hft-engineering/chapter-01-hft-fundamentals-ecosystem/1.2-热路径与冷路径.md)。

无锁队列：先理解 [14 §7.2](../14-hft-engineering/chapter-07-lockless-data-structures-memory-layout/7.2-无锁FIFO队列.md)，再决定手写还是 `crossbeam-queue`。`unsafe` 只包在 queue 内部。demo 用 `Vec<Event>` 代替环（`replay.rs:10-11` 注释：「不手写无锁环（避免 `unsafe`）」），就是为了把这一课推迟。

### Rust 所有权替换了什么

C++ 版（P10）靠**约定**保证「只有引擎线程碰 Book」——约定靠 review 维持，破了不报错。Rust 版把这个约定变成了类型：`Engine::handle(&mut self, ...)` 的可变借用同时只能存在一个，想在另一个线程里碰 `book`，编译器直接拒绝。这不是风格差异，是把一类数据竞争 bug 从「运行时偶发」挪到「编译期必现」。

代价也真实存在：跨线程共享就要 `unsafe` 或无锁队列（上面那节），而那是 demo 刻意推迟的课。

---

## 线程切分（最小）

| 线程 | 职责 | 核 |
|------|------|-----|
| MD | 收包、解析、push 环 | 可与网卡同 NUMA |
| Engine | Book + 策略 + 风控 | isolcpus 独占（生产） |
| Log | 从另一个环 pop 文本 | 随便哪个核 |

demo 和笔记本 WSL **不绑核、不双线程**。生产怎么绑看 [14 Ch5](../14-hft-engineering/chapter-05-os-kernel-tuning/README.md)。

---

## HFT/嵌入式关联

- **「一拍的边界」就是嵌入式里的控制周期边界**：传感器一串中断都处理完才跑一次控制律，和 `run_strategy` 标志是同一个概念。在半更新状态上跑控制律，两种系统都会出鬼结果。
- **热路径不分配**（`with_capacity` 预分配、不加锁、不调系统调用）与裸机 ISR 上下文约束同构：能进热路径的操作集合，由「最坏情况延迟可推理」决定，不由平均性能决定。
- **p99 思维**在嵌入式对应 WCET：实时系统按最坏情况设计，不按典型情况。两者共享同一条纪律——尾部指标才是设计指标。

---

## 代码自测

<details>
<summary>1. 为什么别人的市场冲击必须先在簿上入账（步骤②），再撤我们的旧报价（步骤⑤）？</summary>

冲击可能已经吃掉了我们的旧挂单，`on_fill` 会先改库存。若顺序反过来，撤单会撤一张已经成交的单（失败但静默），且策略按未更新的库存报价——两个错误都无声无息。
</details>

<details>
<summary>2. <code>LatencyHist::add</code> 为什么不排序？排序放哪了？</summary>

排序 O(n log n) 不能进热路径。`add` 只做 `Vec::push`（O(1) 摊销，且容量预分配不 realloc）；排序推迟到报表阶段 `percentile()` 里 clone 后 `sort_unstable()`——那是冷路径，随便花。
</details>

<details>
<summary>3. demo 报表里的 p99 是「系统延迟」吗？</summary>

不是。它只量 quote+risk+submit 这段纯计算（`engine.rs:123`→`:152`），不含网络收发、不含 SPSC 排队、不含内核协议栈。报表自己也印了这条提醒（`engine.rs:209`）。拿它当系统延迟会低估真实 T2T 一到两个数量级。
</details>

<details>
<summary>4. Rust 版相对 C++ 版，「只有引擎线程能改 Book」这条约束强在哪？</summary>

C++ 靠约定（review 维持，破了不报错）；Rust 靠 `&mut self` 可变借用唯一性，编译期强制。一类数据竞争 bug 从运行时偶发变成编译期必现。代价：跨线程共享必须引入无锁队列或 unsafe。
</details>

---

## 卡住翻哪篇

| 卡住了… | 翻这里 |
|---------|--------|
| C++ 引擎清单 | [14 Ch8](../14-hft-engineering/chapter-08-ultra-low-latency-engine-dev/README.md) |
| 原子与内存序 | [17 atomic](../17-rust-foundation/05-Async-Concurrency-Network/01-atomic/) |
| 无锁队列 | [14 §7.2](../14-hft-engineering/chapter-07-lockless-data-structures-memory-layout/7.2-无锁FIFO队列.md) |
| 本模块引擎 | [`demo/src/engine.rs`](./demo/src/engine.rs) |
