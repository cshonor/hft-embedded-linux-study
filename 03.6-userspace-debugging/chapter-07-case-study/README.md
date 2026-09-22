# Ch7 实战：下单程序全流程调试

> 🔴 精读 · 把前六章串起来

**这一章做什么**：一个「多线程 + 网络 + 共享内存」的迷你下单程序，从写出 bug 到定位修复，走一遍完整调试流程——用 Ch1 的方法论做分诊，用 Ch2–Ch6 的工具逐个击破。

---

## 小节索引

| 小节 | 笔记文件 |
|------|----------|
| 🟢 7.0′ 第一次全流程实战（零起点：九种跑法一张表，四类雷各落哪个工具） | [00-first-full-debug.md](notes/00-first-full-debug.md) |
| 7.1 程序结构（多线程 / 网络 / 共享内存的 bug 埋点） | `notes/01-program-structure.md` |
| 7.2 崩溃 → coredump 回溯定位 | `notes/02-crash-coredump.md` |
| 7.3 竞态 → TSan / gdb 多线程定位 | `notes/03-race-tsan.md` |
| 7.4 泄漏 → valgrind / ASan 定位 | `notes/04-leak-valgrind.md` |
| 7.5 卡住 → strace 定位 | `notes/05-hang-strace.md` |

---

## 可跑的 demo（`code/`）

`code/c7_1_trader.c` 就是 7.1 那个 `trader.c`，做成可编译可跑的文件。
**一个程序九种跑法**，覆盖本章全部五节的症状：

| 跑法 | 关键输出 | 退出码 |
|------|----------|--------|
| 基线 | `total matched qty = 40100`；malloc 200 / free 200 / 仍存活 0 | **0** |
| 基线 + TSan | `total = 40100` 正确，但 TSan 报 **1 条** `g_running` 竞争 | **66** |
| 基线 + TSan + `-DFIX_RUNNING` | TSan **静默** | **0** |
| 基线 + ASan | `total = 40100`，无泄漏报告 | **0** |
| `-DBUG_CRASH`（无栈保护） | `Program terminated with signal SIGSEGV (11)` | **139** |
| `-DBUG_CRASH` + `-fstack-protector-all` | `*** stack smashing detected ***` | **134** |
| `-DBUG_RACE` + TSan | `total = 80200`（= 2×40100）；TSan 报 **3 条** | **66** |
| `-DBUG_LEAK` + ASan | `LeakSanitizer: 6400 byte(s) leaked in 200 allocation(s)` | **1** |
| `-DBUG_HANG` | 看门狗 alarm 判定「自死锁实锤」 | **4** |

> ⚠️ 表里第 2、3 行是本次改写最重要的发现：**笔记里的「正确版本」本身就有数据竞争**
> —— `volatile int g_running` 不是同步原语，feed 无锁写它、match 持锁读它。
> 加上 `-DFIX_RUNNING`（换成 `atomic_int` + 退出条件挪进锁内）TSan 才静默。
> 详见 [`code/README.md`](code/README.md) 第 2 节。

另外本版本比笔记里的 `trader.c` 多了两样东西，都是为了「在容器里也能演示」：
`alarm(3)` 看门狗（容器没有 `timeout` 命令，卡死进程没法收场）和订单分配计数
（没有 valgrind 时，自己数 `malloc/free` 是唯一可用的降级手段）。

详见 [`code/README.md`](code/README.md)（九种跑法的完整输出 + 四个踩坑记录）。

---

## HFT 关联

- **综合演练**：真实交易系统的 bug 往往跨类（竞态引发崩溃、泄漏伴随卡死），本章练习「多工具接力」而非单工具死磕；
- **沉淀调试套路**：把「症状 → 定性 → 选工具 → 定位 → 修复 → 回归」固化成肌肉记忆。
