# 4.4 BCC 的工具

> 底本：《BPF之巅》第 4 章 BCC（印刷 p91–136），4.4 节

## 内容详解

### 图 4-2：BCC 工具全景

按资源域划分（观测目标 × 事件源）：

| 资源域 | 代表工具 |
|--------|----------|
| CPU/调度 | `runqlat`、`runqlen`、`offcputime`、`profile`、`cpudist`、`pidstat` |
| 内存 | `memleak`、`oomkill`、`funclatency` |
| 文件系统 | `fileslower`、`filetop`、`vfsstat`、`writeback` |
| 块设备 I/O | `biolatency`、`biosnoop`、`bitesize`、`bfq*.py` |
| 网络 | `tcpconnect`、`tcpaccept`、`tcpretrans`、`tcptop`、`netlatency` |
| 安全/其他 | `execsnoop`、`opensnoop`、`killsnoop`、`statsnoop`、`syscount` |

表 4-1 将"重点工具"按主题 × 章节索引（正文各章按观测目标逐个展开，第 6–14 章）。

### 单一用途 vs 多用途：设计哲学

**单一用途工具遵循 UNIX 哲学：每个工具做好一件事。**

以 `opensnoop -h` 为例——一个单用途工具的完整参数面：

```
usage: opensnoop [-h] [-T] [-x] [-p PID] [-t TID] [--cgroup CGROUP]
                 [-n NAME] [-d DURATION] [-e] [--print-quantize]

Trace open() syscalls
examples:
./opensnoop           # trace all open() syscalls
./opensnoop -T        # include timestamps
./opensnoop -x        # only show failed opens
./opensnoop -p 181    # trace this PID only
```

| 优点 | 代价 |
|------|------|
| 输出即答案，无需再拼管道 | 70+ 个名字要认 |
| man 8 手册 + 示例文件齐全 | — |

**多用途工具（4 个）**，表 4-2：

| 工具 | 回答什么 | 输出形态 | 适合事件频率 |
|------|----------|----------|--------------|
| `funccount` | 谁被调了多少次？ | 计数表 | **高** |
| `stackcount` | 哪些栈路径触发了事件？ | 栈 + 计数 → 火焰图 | **中高** |
| `trace` | 每次事件的参数/返回值？ | **逐行打印** | **低** |
| `argdist` | 参数/返回值分布？ | 频率表或 2 的幂直方图 | **中高** |

后四节（4.5–4.8）逐个精讲。

## HFT 关联

- 高频事件（收发包、锁、syscall）**永远用聚合类**（funccount/argdist/单用途直方图工具）；`trace` 只用于低频事件或排障窗口——逐行打印走 perf 缓冲区 + 用户态格式化，高频时丢事件还拖慢目标进程。
- 认工具先看 `man 8 <tool>` 与 `examples/`——**OVERHEAD 一节决定能否常驻生产**。

## 陷阱

- ⚠️ `trace` 挂到高频函数（如 `tcp_sendmsg`）上 = 人为制造性能事故；先 funccount 估频次再决定。
- ⚠️ 单用途工具的 `-x`（只看失败）等过滤是**内核态完成**的，比用户态 grep 便宜得多。

<details>
<summary>自测题</summary>

1. opensnoop 遵循什么设计哲学？它的过滤参数在哪一层完成？
   <details><summary>答案</summary>UNIX 哲学（单一用途做好一件事）；过滤在 BPF 程序（内核态）完成。</details>

2. 四大多用途工具中哪个绝不能用于高频事件？为什么？
   <details><summary>答案</summary>`trace`——逐事件逐行打印，高频时开销与丢事件都不可接受。</details>
</details>
