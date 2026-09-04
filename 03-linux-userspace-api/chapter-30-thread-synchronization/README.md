# TLPI 第 30 章 — Threads: Thread Synchronization

**优先级**：🔴（多线程正确性核心）
**前置**：[Ch29 线程导论](../chapter-29-threads-intro/README.md)
**后置**：[Ch31 线程安全 / TLS](../chapter-31-thread-safety-tsd/README.md)

---

## 小节目录

- [30.1 互斥量：保护共享变量](notes/30.1-protecting-accesses-to-shared-variables-.md) —— 竞态解剖 + mutex API/类型 + futex 内核路径 + 死锁
- [30.2 条件变量：通知状态变化](notes/30.2-signaling-changes-of-state-condition-var.md) —— wait 三步语义 + while 谓词 + 丢唤醒 + condvar↔futex
- [30.3 总结：职责、铁律与成本账本](notes/30.3-summary.md) —— 全章速览 + 反直觉真相 + 成本账本
- [30.4 练习](notes/30.4-exercises.md) —— 7 题 + 多生产多消费综合题

---

## 章节目标

竞态到底怎么发生（含编译器合并循环的铁证）；`pthread_mutex_*` 全家 + 四种锁类型；futex 内核路径（哈希桶 / plist / 先锁桶再读值 / wake_q 锁外唤醒）；死锁四条件与防御；条件变量三步原子语义、while 谓词三种故障、丢唤醒时序；生产者-消费者有界队列与优雅关闭。

**一句话内核视角**：pthread 的 mutex 和 condvar 是同一套 futex 设施的两个用户态前端——锁的状态字就在用户内存（`__lock`/`__wseq`），无竞争时零系统调用（7 ns 一条 CAS），有竞争才进内核睡哈希桶；唤醒按优先级（plist）出队。

---

## 速查：mutex vs cond 职责

| 原语 | 职责 | 无竞争成本 | 有竞争成本 |
|------|------|-----------|-----------|
| mutex | 互斥访问**状态** | 7 ns（用户态 CAS） | 缓存行乒乓 ~70 ns；futex 睡眠 µs 级 |
| cond | 等待**状态变化** | —（配 mutex 用） | signal→wake ≈ 一次 futex 唤醒（WSL2 实测 ≈ 裸 futex）|

---

## 全章生命周期全景

```
        用户态（glibc）                         内核态（futex，v6.6）
┌───────────────────────────┐        ┌────────────────────────────────┐
│ pthread_mutex_lock        │        │ sys_futex → do_futex():85      │
│  CAS(__lock: 0→1) 成功 ✔  │        │                                │
│  失败 → 自旋(ADAPTIVE)    │        │ FUTEX_WAIT:                    │
│       ↓ syscall           │──────▶ │  futex_wait_setup():577        │
│                           │        │   先锁桶→读值≠expected→EAGAIN   │
│ pthread_cond_wait         │        │   值匹配→入队(plist)→schedule   │
│  wseq++ / 代(G1/G2)       │        │                                │
│  FUTEX_WAIT(cond 词)      │──────▶ │ FUTEX_WAKE: futex_wake():143   │
│                           │        │  waiters==0 早退(连桶锁都不拿)   │
│ pthread_cond_signal       │        │  桶锁→plist匹配key→wake_q      │
│  FUTEX_WAKE(1)            │──────▶ │  锁外 wake_up_q → 调度器       │
│                           │        │                                │
│ unlock: __lock 1→0        │        │  唤醒者醒来→抢回 mutex→while 谓词│
│  旧值==2 才 WAKE          │        │                                │
└───────────────────────────┘        └────────────────────────────────┘
```

---

## 五个反直觉真相

| # | 真相 |
|---|------|
| 1 | 编译器能把 50 万次 `++` 合并成一条 `addq $0x7a120`——普通变量测竞态测了个寂寞（实测 volatile 版丢失 52%）|
| 2 | 单核 0% 丢失 ≠ 安全，只是窗口 ~3ns 撞上时间片切换的概率 ≈ 10⁻⁶ |
| 3 | 无竞争 mutex（7.1 ns）比 `atomic_fetch_add`（13.8 ns）还便宜 |
| 4 | condvar 唤醒延迟 ≈ 裸 futex 唤醒延迟（实测两者几乎相等）——延迟地板在内核，用户态优化不动 |
| 5 | signal 不保存信号、只保证"至少一个"——正确性永远靠 while 谓词，不靠唤醒语义 |

---

## 实验清单

1. 无保护 `volatile ++` 竞态复现（跨核丢失 ~50%）✅ 30.1 demo1
2. mutex 修复 + 计数与耗时对比 ✅ 30.1 demo2
3. `ERRORCHECK` 抓非法操作（EDEADLK / EPERM）✅ 30.1 demo3
4. ABBA 死锁 + timedlock 探测 ✅ 30.1 demo4
5. objdump 验证编译器合并循环 ✅ 30.1 §1.3
6. condvar 严格握手 ping-pong ✅ 30.2 demo1
7. 不持锁 signal 的丢唤醒（timedwait 探测）✅ 30.2 demo2
8. broadcast 惊群 + while 自动分流 ✅ 30.2 demo3
9. 多生产多消费有界队列 + 优雅关闭 + 统计校验 ✅ 30.4 综合题
10. （选）裸 futex 系统调用基线（空 WAKE 203 ns、乒乓延迟）✅ 30.1 §5

---

## 易错清单

1. 普通变量测竞态 → 编译器合并，丢失率假 0%
2. `if` 谓词 → 虚假唤醒 / 惊群 / 错醒三种死法
3. 不持锁改谓词或 signal → 丢唤醒（高负载偶发，最难查）
4. `while` 谓词忘了带关闭条件 → close 后等待者睡死
5. 同线程二次 lock 普通锁 → 自锁死锁
6. 非持有者 unlock → UB（ERRORCHECK 下才报 EPERM）
7. 多锁顺序不一致 → ABBA 死锁
8. Pthreads 错误码查 `errno` → 永远查不到（从返回值取）
9. 拷贝 mutex / cond → UB
10. 忘 destroy / 忘 unlock 早退分支
11. 临界区里做 IO、malloc、printf → 峰值延迟失控
12. `RECURSIVE` 锁当万能药 → 掩盖设计问题
13. 以为 signal 存信号 / signal 唤醒"恰好一个" → 都不是
14. 多核竞争下无脑用 spinlock → 自旋风暴反而更慢（实测 81 vs 69 ns）
15. timedlock/timedwait 误用相对时长或 REALTIME 基准 → 用**绝对时间**；时钟跳变环境配 MONOTONIC

---

## 背诵卡

| # | 要点 |
|---|------|
| 1 | 共享可变数据必须同步；无同步 = UB（编译器也参与捣乱）|
| 2 | 测竞态必须 volatile/_Atomic |
| 3 | 错误码从 Pthreads 返回值取 |
| 4 | mutex 快路径 = 一条 CAS（7 ns，零系统调用）|
| 5 | futex 无竞争不出内核；等待队列在内核、状态字在用户内存 |
| 6 | `futex_wait_setup`：先锁哈希桶、再读值（防丢唤醒的最后防线）|
| 7 | `futex_wake`：无等待者早退；wake_q 延迟到锁外唤醒 |
| 8 | cond 必须**永远**配 mutex |
| 9 | 谓词永远 `while (!pred) wait` |
| 10 | wait = 原子放锁入睡 + 醒来重拿锁 |
| 11 | signal 至少一个 / broadcast 全部；都不保存信号 |
| 12 | 一个 cond 对应一个谓词 |
| 13 | 关闭队列：持锁置 closed + 双 broadcast |
| 14 | 多锁全局统一顺序；timedlock 当死锁探测器 |
| 15 | 唤醒延迟地板 = futex + 调度（µs 级）；热路径要么不睡要么无锁 |

---

## 参考

- Kerrisk · TLPI Ch30
- `man 3 pthread_mutex_lock` · `man 3 pthread_cond_wait`
- futex 语义原始文献：Rusty Russell, *Fuss, Futexes and Furwocks* (OLS 2002)；Drepper, *Futexes Are Tricky* (2011)
- 内核源码（v6.6）：`kernel/futex/{core,waitwake,pi,requeue,syscalls}.c`、`kernel/futex/futex.h`、`include/uapi/linux/futex.h`
