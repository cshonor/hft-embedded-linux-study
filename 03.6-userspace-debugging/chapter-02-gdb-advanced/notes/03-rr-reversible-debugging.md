# 2.3 rr 可逆调试（record / replay / reverse-*）

> 🔴 精读 · 偶发崩溃的「倒带重放」

## 本节要点

最头疼的 bug 是**偶发**的：跑一百次崩一次，你设断点它就崩得恰到好处地「不崩了」。传统 gdb 只能**向前**走，错过了就得重跑。rr（Record & Replay，Mozilla 出品）把一次运行**完整录下来**，之后可以像看录像一样**倒退**——`reverse-continue` 从崩溃点倒回根因，`reverse-next` 反向单步，一次都不用重跑。

## 可逆调试解决什么

| 传统 gdb | rr |
|----------|-----|
| 只能前进，错过现场要重跑 | 录一次，前进后退随意 |
| 偶发 bug 复现靠运气 | 录到的那一次永远可重放 |
| 竞态「一调试就消失」（海森堡 bug） | 重放确定性，竞态被钉死 |
| 崩溃后只能看 core 快照 | 崩溃前后完整执行流都在 |

核心思想一句话：**程序的非确定性（syscall 结果、信号时机、线程交错、时钟）是 bug 复现难的根源；rr 在 record 阶段把这些全部记下来，replay 阶段注入同样值，使重放完全确定，因此时间可以倒流。**

## 安装与基本流程

```bash
# 安装
apt install rr          # Debian/Ubuntu
dnf install rr          # Fedora

# 基本流程：record 录 → replay 放
rr record ./orderbook           # 录一次运行（程序正常跑完或崩溃）
# rr: Saving execution to trace directory ...

rr replay                       # 重放，进入 gdb 界面
```

```gdb
# rr replay 打开的就是一个 gdb，只是多了反向命令
(gdb) continue                  # 正常跑到崩溃
Program received signal SIGSEGV, Segmentation fault.

(gdb) reverse-continue          # 倒回！回到崩溃前的上一个事件
(gdb) reverse-next              # 反向单步（往回退一行）
(gdb) reverse-step              # 反向 step（往回退，含进入函数）
(gdb) reverse-finish            # 反向 finish（退回调用者）
```

## 实战：orderbook 段错误倒带

用 1.1 的 `orderbook.c`（`*p = 42` 段错误）。传统做法是设断点逐步逼近；rr 直接从崩溃点倒回去：

```bash
rr record ./orderbook
# id=3 price=99.50 qty=-1
# ...
# Segmentation fault (core dumped)
# rr: Saving execution to trace directory ...

rr replay
(rr) continue
Program received signal SIGSEGV, Segmentation fault.
0x00005555555551a7 in main () at orderbook.c:28
28          *p = 42;

(rr) reverse-continue          # 倒回上一个执行事件
# 程序停在崩溃前，比如停在 print_orders 返回处

(rr) reverse-next              # 逐行往回退
27          int *p = NULL;     # ← 退到这里，看到 p 是怎么变成 NULL 的

(rr) print p
$1 = (int *) 0x0               # 坐实：p 从未被赋值，一直是 NULL
```

更进一步——反向观察一个变量的值变化：

```gdb
(rr) watch -l p                # 观察 p（-l = 硬件观察点）
(rr) reverse-continue          # 反向跑到 p 上一次被写的地方
# 停在 p 被改成 NULL 的那一行，直接看到「谁、哪一行」把 p 写成了 NULL
```

> `watch` + `reverse-continue` 是 rr 的杀手锏组合：正向 watch 找「谁改坏了我」，反向 watch 找「我是什么时候被改坏的」，两条路都能走到根因。

## rr 的原理：捕获哪些非确定性

程序重放之所以能确定，是因为 rr 在 record 阶段把**所有非确定性来源**记录了下来：

| 非确定性来源 | rr 怎么处理 |
|--------------|-------------|
| **syscall 结果**（read 返回多少、select 返回什么） | record 时 ptrace 接管，记下每个 syscall 的返回值/参数副作用；replay 时直接注入记录值，不再真读 |
| **信号递送时机** | 记下信号何时、发给谁；replay 在相同位置重放 |
| **共享内存竞争**（多线程） | record 用固定调度顺序（round-robin），把线程交错也记录；replay 按同样顺序执行 |
| **时钟**（`gettimeofday`/`clock_gettime`） | 记下返回值，replay 时返回同样时间 |
| **`rdtsc`/CPU 计数** | record 时被捕获或让程序读到虚拟值 |
| **硬件中断** | record 阶段通过 perf 事件 + ptrace 精确控制 |

record 的代价：**性能下降约 1.2×–2×**（主要是 syscall 都要过 ptrace），内存占用上升。但对「录一次、反复倒带」的调试模式完全可接受。

## 局限与陷阱

| 局限 | 说明 |
|------|------|
| 需要性能计数器 | 依赖 perf 事件，某些**云 VM/容器**默认禁了 perf_event_open，会报错 |
| 不支持某些场景 | io_uring、部分 GPU/驱动相关 syscall 支持不全（随版本改善） |
| record 不能跨机 replay | trace 目录与 CPU 微架构相关，通常要同机重放 |
| 大程序 record 慢 | syscall 密集的程序（如高吞吐网络）record 开销可能 >2× |
| 不能改程序后 replay | record 和 replay 必须是同一二进制（改了代码 trace 就废了） |

```bash
# 常见报错：perf 被禁
rr record ./prog
# rr: error: ... perf_event_open failed: Operation not permitted
# 解决：echo -1 > /proc/sys/kernel/perf_event_paranoid 或加 CAP_SYS_ADMIN
```

## rr 与 scheduler-locking 的配合

rr replay 时，`scheduler-locking replay` 是专有模式——它让 gdb 在反向单步时也保持 rr 的确定性调度，避免重放时线程顺序漂移：

```gdb
(rr) set scheduler-locking replay
```

## HFT 关联

1. **偶发错单/崩溃的终极解法**：交易系统「偶尔崩一次、复现不了」最致命，rr 录到的那一次可无限重放、倒带，把偶发 bug 变成确定性可查。
2. **海森堡竞态**：加了调试输出/断点竞态就消失（时序被打乱），rr 的确定性重放让竞态「钉死」，`reverse-continue` + `watch` 直接追到写坏数据的源头。
3. **低延迟敏感的 record 成本**：syscall 密集的热路径 record 开销大，可只对**可疑模块**的复现程序（缩小复现范围）record，而非整个交易引擎。
4. **回归测试的复现载体**：把 rr trace 归档，崩溃场景可反复重放验证修复是否真正生效。

---

<details>
<summary>自测题（点击展开）</summary>

**Q1:** rr 为什么能做到「时间倒流」，而普通 gdb 不行？

> 普通 gdb 只记录断点处的状态，没有记录「执行路径上每个非确定性事件的取值」，所以无法倒推。rr 在 record 阶段用 ptrace 把 syscall 返回值、信号时机、线程调度顺序、时钟等**全部非确定性来源**记成 trace；replay 时注入同样的值，执行完全确定，于是可以从任意点反向推导。

**Q2:** 「海森堡 bug」是什么？rr 怎么治它？

> 海森堡 bug 指「一加调试/一打断点就消失」的 bug——因为调试改变了时序，掩盖了竞态。rr 治它的关键在于：record 阶段记录的是一次**真实、无调试干扰**的运行（开销虽在但调度逻辑不变），replay 用确定性调度重放，竞态被忠实还原且可反复观察。

**Q3:** `watch` + `reverse-continue` 组合为什么强大？

> 正向 `watch` 回答「谁把我改坏了」（往前走找下一次写），反向 `watch` + `reverse-continue` 回答「我是什么时候、被哪一行改成这个坏值的」（往后走找上一次写）。两者结合能从「现场」双向逼近「根因」，尤其适合「变量不知何时被写坏」这类问题。

**Q4:** record 的 trace 能拷到别的机器上 replay 吗？为什么？

> 通常不能。trace 里记录了与具体 CPU 微架构相关的信息（性能计数器、指令执行细节），跨机（甚至同机不同 CPU 代际）重放可能不精确或失败。标准做法是同机 record + replay。

**Q5:** rr record 一个高吞吐网络程序为什么可能「慢得多」甚至失败？

> 因为每个 syscall 都要过 ptrace 记录结果，高吞吐网络程序 syscall 密度极高，ptrace 拦截成本被放大，record 开销可能远超 2×。此外部分网络相关机制（如 io_uring）支持不全。对策：缩小复现范围，只对最小复现程序 record，而非整机全量。

</details>

## 交叉引用

- [2.1 多线程调试](01-thread-debugging.md)
- [2.2 attach 运行中进程](02-attach-running-process.md)
- [1.2 断点与观察点](../../chapter-01-gdb-basics/notes/02-breakpoints.md)
- [03.6 模块导读](../../README.md)
