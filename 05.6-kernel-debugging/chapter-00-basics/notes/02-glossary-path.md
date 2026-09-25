# 0.3 术语速查表 + 新手学习路径

> 🟢 零起点 · 当字典查；后半告诉你 12 章按什么顺序啃

## 本节讲什么

前两节碰到的词集中解释一遍（每个词一行 + 去哪章深挖），然后给出三条
「按目标选路线」的学习路径——不是每章都精读，是**按你需要什么来啃**。

## 术语速查表（按出场顺序）

| 术语 | 一句话解释 | 深挖 |
|------|-----------|------|
| **dmesg** | 把内核日志缓冲区倒出来的命令；内核世界的 stdout | 0.2 |
| **printk** | 内核版的 printf，带 8 级日志级别（0=EMERG … 7=DEBUG） | [Ch3](../../chapter-03-printk/) |
| **dynamic debug** | 不重编内核就能逐条开关的 pr_debug；`/proc/dynamic_debug/control` | [Ch3](../../chapter-03-printk/) |
| **Oops** | 内核版的段错误报告：一屏现场（死因/RIP/寄存器/Call Trace），通常只杀死出错任务 | [Ch7](../../chapter-07-oops/) |
| **panic** | 内核宣布死亡：全机冻结，只能重启；快速失败哲学 | [Ch10](../../chapter-10-panic-lockup/) |
| **Call Trace** | Oops 里「怎么走到死地的」调用链，从下往上读 | [Ch7](../../chapter-07-oops/notes/03-call-trace-analysis.md) |
| **RIP** | x86_64 的指令指针寄存器 = 「当前执行到哪」；Oops 里用它定位死点 | [Ch7](../../chapter-07-oops/notes/02-register-dump.md) |
| **Tainted** | 内核「被污染」标记（加载了非 GPL 模块、发生过 Oops 等）；日志里 `Tainted: G O` 那串 | [Ch7](../../chapter-07-oops/) |
| **KASAN** | Kernel Address SANitizer：编译期插桩的内存错误检测器（越界/UAF），要重编内核 | [Ch5](../../chapter-05-memory-debug-1/) |
| **UBSAN / SLUB debug / kmemleak** | 同族：未定义行为 / slab 分配器校验 / 内核内存泄漏检测，都要重编内核 | [Ch5](../../chapter-05-memory-debug-1/) / [Ch6](../../chapter-06-memory-debug-2/) |
| **LOCKDEP** | 锁依赖验证器：开发期证明「你的加锁顺序永远不会死锁」，要重编内核 | [Ch8](../../chapter-08-lock-debug/) |
| **KCSAN** | 数据竞争检测器（并发访问同一内存无同步） | [Ch8](../../chapter-08-lock-debug/) |
| **ftrace** | 内核自带的函数级追踪器：谁调了谁、每个函数耗时多少；不重编内核可用 | [Ch9](../../chapter-09-ftrace/) |
| **trace-cmd / KernelShark** | ftrace 的命令行封装 / 图形界面 | [Ch9](../../chapter-09-ftrace/) |
| **kprobes** | 运行时在任意内核函数上插探针（抓参数/返回值），不重编内核 | [Ch4](../../chapter-04-kprobes/) |
| **KGDB** | 源码级单步调试内核：需要第二台机器 + 串口/网线，gdb 远程连 | [Ch11](../../chapter-11-kgdb/) |
| **lockup（soft/hard）** | 挂死检测：CPU 长时间不调度（soft）/ 中断都不响应（hard）；watchdog 负责 | [Ch10](../../chapter-10-panic-lockup/) |
| **debug kernel vs production kernel** | 开了 KASAN/LOCKDEP 等的内核（慢、大）vs 线上性能内核；调试用前者 | [Ch1](../../chapter-01-introduction/) |

> 标了「要重编内核」的工具 = **侵入性高**；标了「不重编可用」的（printk/ftrace/kprobes）
> = 新手先掌握的**低侵入三件套**。

## 新手学习路径：三条路线按目标选

### 路线 A：「我只是想看懂服务器日志」（最短，半天）

```
本章 0.1 → 0.2 → Ch3 printk → Ch7 Oops（只读 07.1/07.2/07.3）
```
产出：看得懂 dmesg 里的报错与 Oops 大意，能在排障群里说出人话。

### 路线 B：「我要写内核模块/驱动」（主线路径，1-2 周）

```
本章 → Ch3 printk（动手）→ Ch7 Oops（精读全章）→ Ch5 KASAN（重编一次内核）
→ Ch8 LOCKDEP → Ch9 ftrace → Ch4 kprobes
```
产出：模块崩了自己能定位到源码行；并发 bug 有机器帮你抓。
Pi 5 全程可跑（Ch1 的 04 节讲怎么在 ARM64 上重编出 debug kernel）。

### 路线 C：「HFT 定制内核旁路/低延迟模块」（完整线）

```
路线 B 全部 → Ch10 panic/lockup → Ch11 KGDB（串口连 Pi 5）→ Ch6/Ch12 选读
```
产出：生产级排障能力——挂死检测、源码级单步、覆盖率/模糊测试可选。

### 一条纪律（全路线通用）

**先低侵入、后高侵入；先观测、后尸检；能 printk 定位的问题不重编内核。**
每往「重」一级走之前，问一句：上一级的手段穷尽了吗？

## 与后续衔接

路径定了就出发：路线 A/B 的第一站都是 [Ch3 printk](../../chapter-03-printk/)；
如果还想先看故事找感觉，Ch1 的调试历史案例（Patriot/Ariane 5）当睡前读物。
