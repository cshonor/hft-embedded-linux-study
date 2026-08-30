# 1.5 BPF 跟踪的能见度

> 底本：《BPF之巅》中文版 1.5 节（PDF p47–48）

## 核心主张

BPF 跟踪可以在**整个软件栈**范围内提供能见度：生产环境**立刻部署、不重启系统、不以特殊方式重启应用**。作者类比：像医学 X 光——需要检查哪个内核组件、设备、应用库时，能以前所未有的方式看到其内部运作，而且是**生产环境现场直播**。

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

## 表 1-1：传统工具 vs BPF 跟踪

| 组件 | 传统分析工具 | BPF 跟踪 |
|---|---|---|
| 语言运行时应用（Java/Node.js/Ruby/PHP） | 运行时调试器 | 是（运行时支持的情况下） |
| 编译型应用（C/C++/Golang） | 系统调试器 | 是 |
| 系统库 /lib/* | ltrace(1) | 是 |
| 系统调用接口 | strace(1), perf(1) | 是 |
| 内核（调度器、文件系统、TCP、IP 等） | — | **是，且更加详细** |
| 硬件（CPU 核心、设备） | perf、sar、/proc 计数器 | 是（直接或间接） |

定位关系：传统工具是性能分析的**起点**，BPF 跟踪做更深入的调查（第 3 章给出结合两者的 60 秒分析流程）。

---

### HFT 关联

这张全景图就是**交易路径逐层计时**的选型地图：行情网卡（nettxlat/skbdrop）→ IP/TCP（tcpretrans/tcpwin）→ 套接字（soconnlat/sormem）→ VFS（vfsstat）→ 调度（runqlat/offcputime）。每层的工具名已在图上，遇到延迟问题按层下钻即可，不需要发明轮子。
