# TLPI 第 32 章 — Threads: Thread Cancellation

**优先级**：🟠（可取消工作线程；资源与锁安全）
**前置**：[Ch29 线程导论](../chapter-29-threads-intro/README.md) · [Ch30 同步](../chapter-30-thread-synchronization/README.md) · [Ch31 TSD/TLS](../chapter-31-thread-safety-tsd/README.md)
**后置**：[Ch33 线程更多细节](../chapter-33-threads-further/README.md)

---

## 小节目录

- [32.1 取消线程](notes/32.1-canceling-a-thread.md) —— 请求≠终止；tgkill/SIGCANCEL 内核链路；join 验证
- [32.2 取消状态与类型](notes/32.2-cancellation-state-and-type.md) —— 2×2 矩阵；临界区 DISABLE 包裹
- [32.3 取消点](notes/32.3-cancellation-points.md) —— "可能睡死的函数"；glibc 2.28 包装层改革
- [32.4 主动检查 `pthread_testcancel`](notes/32.4-testing-for-thread-cancellation.md) —— 手动检查站三原则
- [32.5 清理处理器](notes/32.5-cleanup-handlers.md) —— 宏配对 + LIFO + return 不展开
- [32.6 异步取消](notes/32.6-asynchronous-cancelability.md) —— 三宗罪；唯一合法场景画像
- [32.7 总结](notes/32.7-summary.md) —— 五步生命周期 + 铁律 + 排查表

---

## 章节目标

`pthread_cancel` 的异步请求语义（join 拿 `PTHREAD_CANCELED`）；状态/类型 2×2 组合；取消点的定义与 glibc 2.28 机制变革；`pthread_testcancel` 检查站纪律；cleanup 栈（宏本质、LIFO、return 陷阱、持锁睡眠保险）；异步取消为什么禁用；HFT 的协作退出替代。

**一句话内核视角**：取消 = glibc 在目标线程置标志 + `tgkill(tgid, tid, SIGCANCEL=32)` 踹醒阻塞中的系统调用（内核 `do_send_specific():3935`，tgkill 双重校验防 PID 复用误杀）+ 目标在取消点/系统调用包装层自查标志后展开清理并 `pthread_exit`。

---

## 全章生命周期全景

```
pthread_cancel(B)
  │ ① glibc: 置标志 + tgkill(SIGCANCEL=32)          ← 内核 do_send_specific()
  ▼
[挂起的请求] ──DISABLE──▶ 继续等
  │ ENABLE+DEFERRED
  ▼
② 取消点（read/sleep/cond_wait/testcancel…）        ← glibc 2.28+: syscall 包装层
  ▼
③ cleanup 栈 LIFO 展开（32.5） + TSD 析构（31.3）
  ▼
④ pthread_exit(PTHREAD_CANCELED)
  ▼
⑤ pthread_join → res == PTHREAD_CANCELED（死亡证明）
```

---

## 实验清单

1. cancel 睡眠线程 + join 验证 `PTHREAD_CANCELED` ✅ 32.1 demo1
2. DISABLE 屏蔽：3 段关键工作跑完才死 ✅ 32.1 demo2
3. 纯计算循环杀不死 → testcancel 补位 ✅ 32.1 demo3
4. cleanup LIFO 逆序 + pop(1) 手动路径 ✅ 32.5 demo1
5. cond_wait 被取消 → cleanup 解锁（主线程随后成功拿锁验证）✅ 32.5 demo2
6. DEFERRED 杀不死 vs ASYNC 秒杀同循环对照 ✅ 32.6
7. （选）read() 阻塞中被取消 ✅ 32.3 补充片段

---

## 易错清单

1. 以为 cancel = 立即终止 → 不 join 就收资源（僵尸/泄漏）
2. 纯计算循环不加 testcancel → 杀不死
3. 临界区 DISABLE 恢复成硬编码 ENABLE → 拆掉上游保护
4. cleanup push 后直接 return → 编译错（宏花括号）——读懂这是保护
5. 以为 return 会展开 cleanup → POSIX 明确不展开
6. cleanup push 放在 lock 前 → 取消时 unlock 未持有的锁（UB）
7. ASYNC 用在持锁/碰堆的线程 → 死锁/堆损坏
8. join 无超时的关停序列 → 一个卡死全线挂死
9. pthread_sigmask 挡了 32 号（SIGCANCEL）→ 取消请求被屏蔽
10. 拿 `kill(tid)` 杀线程 → tid 不是进程；用 `pthread_cancel` 或 `tgkill`

---

## 背诵卡

| # | 要点 |
|---|------|
| 1 | cancel 是请求；join+PTHREAD_CANCELED 是死亡证明 |
| 2 | glibc 用 tgkill 发 SIGCANCEL(32)；tgkill 双重校验防 PID 复用 |
| 3 | DISABLE 挂起不丢弃；请求不可撤销 |
| 4 | 类型永远 DEFERRED；ASYNC 生产禁用 |
| 5 | 取消点 ≈ 可能睡死的函数；纯计算=永无取消点 |
| 6 | testcancel 放块间安全边界；密度=取消延迟上界 |
| 7 | cleanup push/pop 是宏：同块配对 |
| 8 | 触发：cancel/pthread_exit/pop(1)；return 不触发 |
| 9 | LIFO 逆序 = 资源获取逆序释放 |
| 10 | 持锁睡 condvar 必挂 cleanup 保险 |
| 11 | push 在 lock 之后（解锁才有东西可托底）|
| 12 | glibc 2.28+ 取消点=syscall 包装层，清单只是下界 |
| 13 | 杀不死五连查：取消点/ENABLE/join/掩码/忙等 |
| 14 | HFT 标准：协作退出（原子标志+condvar），cancel 只做兜底 |
| 15 | 无界计算的超时兜底用进程边界（fork+kill）|

---

## 参考

- Kerrisk · TLPI Ch32
- `man 3 pthread_cancel` · `man 3 pthread_cleanup_push` · `man 3 pthreads`（取消点清单）
- glibc 2.28 取消机制重设计（futex-cancel）相关 NPTL 变更说明
- 内核源码（v6.6）：`kernel/signal.c`（`do_send_specific:3935` · `do_tkill:3962` · `tgkill:3988`）
