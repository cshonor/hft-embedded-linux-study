# 1.5 BPF 跟踪的能见度

> 底本：《BPF之巅》中文版 1.5 节（PDF p47–48）

## 核心主张

BPF 跟踪可以在**整个软件栈**范围内提供能见度：生产环境**立刻部署、不重启系统、不以特殊方式重启应用**。作者类比：像医学 X 光——需要检查哪个内核组件、设备、应用库时，能以前所未有的方式看到其内部运作，而且是**生产环境现场直播**。

"不重启"的工程含义值得强调：传统内核观测要 `debugfs`/重启开 kprobe、应用级观测要预装 agent 并滚动重启进程。BPF 的探针生命周期是**分钟级热插拔**——出问题的当下就能挂上，不需要预约变更窗口。

## 图 1-2：软件栈全景 × 工具覆盖

原书用一张大图把通用软件栈自上而下分层，每层标注可用工具（此处摘录主干）：

| 栈层 | 代表工具 |
|---|---|
| 应用程序 | mysqld_qslower, dbstat/dbslower, bashreadline, signals, naptime, killsnoop |
| 编程语言运行时 | javathreads, ucalls/uflow/uobjnew/ustat, uthreads/ugc, jnistacks, sslsniff |
| 系统库 | gethostlatency, memleak, threadsnoop |
| 系统调用接口 | execsnoop, exitsnoop, syscount, opensnoop, statsnoop, killsnoop |
| VFS / 文件系统 | vfsstat, vfscount, vfssize, filetop, fileslower, cachestat, ext4/xfs/zfs/btrfs/nfs slower+dist, icstat, dcstat, fsrwstat, filelife, readahead |
| 卷管理器 / 块设备 | biolatency, biosnoop, biotop, bitesize, seeksize, biopattern, mdflush, ioqlat, scsilatency, nvmelatency, bioerr |
| 网络套接字 / TCP/UDP / IP / 网络设备 | sockstat, sofamily, soprotocol, sormem, soconnlat, tcpconnect, tcpaccept, tcplife, tcptop, tcpretrans, tcpsynbl, tcpwin, tcpnagle, tcpreset, udpconnect, skbdrop, netsize, nettxlat, ieee80211scan |
| 调度器 | runqlat, runqlen, runqslower, offcputime, offwaketime, wakeuptime, cpuwalk |
| 虚拟内存 | oomkill, faults, ffaults, hfaults, vmscan, swapin, drsnoop, mmapsnoop, brkstack |

**浏览这张图的意义**：发现自己先前的**分析盲区**——每个栈层都有工具覆盖，没有哪一层是黑盒。

### 这张图怎么用（读图法）

1. **横着读（排障）**：延迟问题先归到某一层，再看该层有什么工具——"工具名已在图上，不需要发明轮子"
2. **竖着读（学习路径）**：本书 Part II 就是按这张图自上而下组织的（ch06 CPU → ch10 网络 → …），每章 = 栈层 × 通用方法论
3. **对号入座（自研系统）**：把自家交易栈画成同构分层图，标注每层已有什么观测、缺什么——多数团队的空白在**系统库层**（gethostlatency 这类）和**驱动/设备层**（nettxlat/skbdrop）

## 表 1-1：传统工具 vs BPF 跟踪

| 组件 | 传统分析工具 | BPF 跟踪 |
|---|---|---|
| 语言运行时应用（Java/Node.js/Ruby/PHP） | 运行时调试器 | 是（运行时支持的情况下） |
| 编译型应用（C/C++/Golang） | 系统调试器 | 是 |
| 系统库 /lib/* | ltrace(1) | 是 |
| 系统调用接口 | strace(1), perf(1) | 是 |
| 内核（调度器、文件系统、TCP、IP 等） | — | **是，且更加详细** |
| 硬件（CPU 核心、设备） | perf、sar、/proc 计数器 | 是（直接或间接） |

注意"内核"一行的**"—"**：传统工具时代，内核内部行为基本靠 `/proc` 计数器 + 想象。这不是工具作者偷懒，而是**传统工具没有安全进入内核内部的机制**——perf 事件点有限、debugfs 需要定制内核。BPF 补的正是这一层的空白，顺带把其他层的观测也统一到同一套可编程框架。

定位关系：传统工具是性能分析的**起点**，BPF 跟踪做更深入的调查（第 3 章给出结合两者的 60 秒分析流程）。

---

### HFT 关联

这张全景图就是**交易路径逐层计时**的选型地图：行情网卡（nettxlat/skbdrop）→ IP/TCP（tcpretrans/tcpwin）→ 套接字（soconnlat/sormem）→ VFS（vfsstat）→ 调度（runqlat/offcputime）。每层的工具名已在图上，遇到延迟问题按层下钻即可，不需要发明轮子。

配套纪律（避免"全景能见度"变成"全景开销"）：

- **不是全挂**——全景图是选型地图不是部署清单；常驻的只有几个聚合型工具（直方图/计数器类），逐事件打印类（*snoop/*slower）只在上手工单时挂
- 交易机上 BPF 工具的输出通道也要管：`fprintf` 逐行输出到终端在风暴期本身就是负载——优先 `@hist()` 直方图、`interval` 定期打印
- 交叉引用：60 秒分析流程见 [Ch3 性能分析](../../chapter-03-performance-analysis/)；各资源域工具详解从 [Ch6 CPUs](../../chapter-06-cpus/) 开始

<details>
<summary>📝 自测题（点击展开）</summary>

1. **表 1-1 中"内核"一行为什么是空的？这反映了传统工具的什么结构性缺陷？**

   <details><summary>参考答案</summary>

   传统工具没有安全进入内核内部执行的机制——能做的只有预先埋好的计数器（/proc）和有限的事件点（perf）。缺陷：内核内部行为的观测依赖内核开发者预先放什么接口，使用者不可编程定制。BPF 的 verifier 把"在内核里跑任意（经验证安全的）小程序"变成可能，才填上这一格。

   </details>

2. **"立刻部署、不重启"对生产排障的价值是什么？对比传统 agent 方案的差异。**

   <details><summary>参考答案</summary>

   传统 agent 要预装、随版本滚动重启进程，观测窗口从"出事后几分钟"退化为"下次变更窗口后"。BPF 探针热插拔意味着问题的第一现场（往往稍纵即逝）就能被捕获——排障最贵的不是分析，是等问题复现。

   </details>

3. **为什么说全景图"不是部署清单"？哪些工具适合常驻、哪些只该手工单次挂载？**

   <details><summary>参考答案</summary>

   每多挂一个逐事件探针，就多一份与事件率成正比的开销。常驻：聚合型（直方图/计数/延迟分桶，如 runqlat/biolatency）；手工：逐事件打印型（*snoop）、带过滤阈值但输出量不可控的（*slower 不带阈值时）。

   </details>

</details>
