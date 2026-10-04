# 《BPF之巅》真实目录（OCR 自中文版扫描底本）

> **来源：** `~/Desktop/hft-local-books/BPF之巅-中文版.pdf` 目录页（PDF 第 20–40 页）逐页 OCR
> **页码映射：** 印刷页码 + 40 = PDF 页码（`bpt_txt/page-NNN.txt`，NNN 从 000 起，故 PDF 页 N → `page-(N-1).txt`）
> **用途：** 逐章笔记的底本对照基准；笔记小节划分以本目录为准

## 全书三部分结构（前言 XII）

- **第 1 部分（第 1–5 章）**：BPF 跟踪背景 — 性能分析、内核跟踪技术、BCC、bpftrace
- **第 2 部分（第 6–16 章）**：BPF 可跟踪目标 — CPU/内存/文件系统/磁盘 I/O/网络/安全/语言/应用程序/内核/容器/虚拟机管理器（支持跳读，各章结构统一）
- **第 3 部分（第 17–18 章）**：其他 BPF 工具 + 提示、技巧和常见问题

---

## 第 1 章 引言（1–15 页）

| 节 | 标题 |
|---|---|
| 1.1 | BPF 和 eBPF 是什么 |
| 1.2 | 跟踪、嗅探、采样、剖析和可观测性分别是什么 |
| 1.3 | BCC、bpftrace 和 IOVisor |
| 1.4 | 初识 BCC：快速上手 |
| 1.5 | BPF 跟踪的能见度 |
| 1.6 | 动态插桩：kprobes 和 uprobes |
| 1.7 | 静态插桩：tracepoint 和 USDT |
| 1.8 | 初识 bpftrace：跟踪 open() |
| 1.9 | 再回到 BCC：跟踪 open() |
| 1.10 | 小结 |

## 第 2 章 技术背景（16–70 页）

| 节 | 标题 |
|---|---|
| 2.1 | 图释 BPF |
| 2.2 | BPF |
| 2.3 | 扩展版 BPF |
| 2.3.1 | 为什么性能工具需要 BPF 技术 |
| 2.3.2 | BPF 与内核模块的对比 |
| 2.3.3 | 编写 BPF 程序 |
| 2.3.4 | 使用 BPF 查看指令集：bpftool |
| 2.3.5 | 使用 bpftrace 查看 BPF 指令集 |
| 2.3.6 | BPF API |
| 2.3.7 | BPF 并发控制 |
| 2.3.8 | BPF sysfs 接口 |
| 2.3.9 | BPF 类型格式（BTF） |
| 2.3.10 | BPF CO-RE |
| 2.3.11 | BPF 的局限性 |
| 2.3.12 | BPF 扩展阅读资料 |
| 2.4 | 调用栈回溯 |
| 2.4.1 | 基于帧指针的调用栈回溯 |
| 2.4.2 | 调试信息 |
| 2.4.3 | 最后分支记录 |
| 2.4.4 | ORC |
| 2.4.5 | 符号 |
| 2.4.6 | 扩展阅读 |
| 2.5 | 火焰图 |
| 2.5.1 | 调用栈信息 |
| 2.5.2 | 对调用栈信息的剖析 |
| 2.5.3 | 火焰图 |
| 2.5.4 | 火焰图的特性 |
| 2.5.5 | 火焰图的变体 |
| 2.6 | 事件源 |
| 2.7 | kprobes |
| 2.7.1 | kprobes 是如何工作的 |
| 2.7.2 | kprobes 接口 |
| 2.7.3 | BPF 和 kprobes |
| 2.7.4 | 关于 kprobes 的更多内容 |
| 2.8 | uprobes |
| 2.8.1 | uprobes 是如何工作的 |
| 2.8.2 | uprobes 接口 |
| 2.8.3 | BPF 与 uprobes |
| 2.8.4 | uprobes 的开销和未来的工作 |
| 2.8.5 | 扩展阅读 |
| 2.9 | 跟踪点（tracepoint） |
| 2.9.1 | 如何添加跟踪点 |
| 2.9.2 | 跟踪点的工作原理 |
| 2.9.3 | 跟踪点的接口 |
| 2.9.4 | 跟踪点和 BPF |
| 2.9.5 | BPF 原始跟踪点 |
| 2.9.6 | 扩展阅读 |
| 2.10 | USDT |
| 2.10.1 | 添加 USDT 探针 |
| 2.10.2 | USDT 是如何工作的 |
| 2.10.3 | BPF 与 USDT |
| 2.10.4 | USDT 的更多信息 |
| 2.11 | 动态 USDT |
| 2.12 | 性能监控计数器（PMC） |
| 2.12.1 | PMC 的模式 |
| 2.12.2 | PEBS |
| 2.12.3 | 云计算 |
| 2.13 | perf events |
| 2.14 | 小结 |

## 第 3 章 性能分析（71–90 页）

| 节 | 标题 |
|---|---|
| 3.1 | 概览（目标 / 分析工作 / 多重性能问题） |
| 3.2 | 性能分析方法论（业务负载画像 / 下钻分析 / USE 方法论 / 检查清单法） |
| 3.3 | Linux 60 秒分析（uptime, dmesg, vmstat, mpstat, pidstat, iostat, free, sar -n DEV, sar -n TCP,ETCP, top） |
| 3.4 | BCC 工具检查清单（execsnoop, opensnoop, ext4slower, biolatency, biosnoop, cachestat, tcpconnect, tcpaccept, tcpretrans, runqlat, profile） |
| 3.5 | 小结 |

## 第 4 章 BCC（91–136 页）

| 节 | 标题 |
|---|---|
| 4.1 | BCC 的组件 |
| 4.2 | BCC 的特性（内核态 / 用户态） |
| 4.3 | 安装 BCC（内核要求 / Ubuntu / RHEL / 其他发行版） |
| 4.4 | BCC 的工具（重点工具 / 工具特点 / 单一用途 / 多用途） |
| 4.5 | funccount（示例 / 语法 / 单行程序 / 帮助信息） |
| 4.6 | stackcount（示例 / 火焰图 / 残缺调用栈 / 语法 / 单行程序 / 帮助信息） |
| 4.7 | trace（示例 / 语法 / 单行程序 / 结构体 / 调试 fd 泄露 / 帮助信息） |
| 4.8 | argdist（语法 / 单行程序 / 帮助信息） |
| 4.9 | 工具文档（man 手册 opensnoop / 示例文件） |
| 4.10 | 开发 BCC 工具 |
| 4.11 | BCC 的内部实现 |
| 4.12 | BCC 的调试（printf 调试 / 调试输出 / 调试标志位 / bpflist / bpftool / dmesg / 重置事件） |
| 4.13 | 小结 |

## 第 5 章 bpftrace（137–190 页）

| 节 | 标题 |
|---|---|
| 5.1 | bpftrace 的组件 |
| 5.2 | bpftrace 的特性（事件源 / 动作 / 一般特性 / 与其他观测工具比较） |
| 5.3 | bpftrace 的安装 |
| 5.4 | bpftrace 工具（重点工具 / 工具特征 / 工具的运行） |
| 5.5 | bpftrace 单行程序 |
| 5.6 | bpftrace 的文档 |
| 5.7 | bpftrace 编程（用法 / 程序结构 / 注释 / 探针格式 / 通配符 / 过滤器 / 动作 / Hello World / 函数 / 变量 / 映射表函数 / 对 vfsread 计时） |
| 5.8 | bpftrace 的帮助信息 |
| 5.9 | bpftrace 的探针类型（tracepoint / usdt / kprobe+kretprobe / uprobe+uretprobe / software+hardware / profile+interval） |
| 5.10 | bpftrace 的控制流（过滤器 / 三元操作符 / if 语句 / 循环展开） |
| 5.11 | bpftrace 的运算符 |
| 5.12 | bpftrace 的变量（内置变量 pid,comm,uid / kstack,ustack / 位置参数 / 临时变量 / 映射表变量） |
| 5.13 | bpftrace 的函数（printf / join / str / kstack+ustack / ksym+usym / kaddr+uaddr / system / exit） |
| 5.14 | bpftrace 映射表的操作函数（count / sum+avg+min+max / histo / lhist / delete / clear+zero / print） |
| 5.15 | bpftrace 的下一步工作（显式区分地址模式 / 其他扩展 / ply） |
| 5.16 | bpftrace 的内部运作 |
| 5.17 | bpftrace 的调试（printf 调试 / 调试模式 / 详情模式） |
| 5.18 | 小结 |

## 第 6 章 CPU（191–254 页）

| 节 | 标题 |
|---|---|
| 6.1 | 背景知识（CPU 基础 / BPF 分析能力 / 分析策略） |
| 6.2 | 传统工具（内核统计 / 硬件统计 / 硬件采样 / 定时采样 / 事件统计与事件跟踪） |
| 6.3 | BPF 工具：execsnoop, exitsnoop, runqlat, runqlen, runqslower, cpudist, cpufreq, profile, offcputime, syscount, argdist+trace, funccount, softirqs, hardirqs, smpcalls, llcstat, 其他工具 |
| 6.4 | BPF 单行程序（BCC 版 / bpftrace 版） |
| 6.5 | 可选练习 |
| 6.6 | 小结 |

## 第 7 章 内存（255–290 页）

| 节 | 标题 |
|---|---|
| 7.1 | 背景知识（内存基础 / BPF 分析能力 / 分析策略） |
| 7.2 | 传统工具（内核日志 / 内核统计信息 / 硬件统计和硬件采样） |
| 7.3 | BPF 工具：oomkill, memleak, mmapsnoop, brkstack, shmsnoop, faults, ffaults, vmscan, drsnoop, swapin, hfaults, 其他工具 |
| 7.4 | BPF 单行程序（BCC / bpftrace） |
| 7.5 | 可选练习 |
| 7.6 | 小结 |

## 第 8 章 文件系统（291–360 页）

| 节 | 标题 |
|---|---|
| 8.1 | 背景知识（文件系统基础 / BPF 分析能力 / 分析策略） |
| 8.2 | 传统工具（df / mount / strace / perf / fatrace） |
| 8.3 | BPF 工具：opensnoop, statsnoop, syncsnoop, mmapfiles, scread, fmapfault, filelife, vfsstat, vfscount, vfssize, fsrwstat, fileslower, filetop, writesync, filetype, cachestat, writeback, destat, dcsnoop, mountsnoop, xfsslower, xfsdist, ext4dist, icstat, bufgrow, readahead, 其他工具 |
| 8.4 | BPF 单行程序（BCC / bpftrace / 单行程序示例） |
| 8.5 | 可选练习 |
| 8.6 | 小结 |

## 第 9 章 磁盘 I/O（361–410 页）

| 节 | 标题 |
|---|---|
| 9.1 | 背景知识（磁盘系统基础 / BPF 分析能力 / 分析策略） |
| 9.2 | 传统工具（iostat / perf / blktrace / SCSI 日志） |
| 9.3 | BPF 工具：biolatency, biosnoop, biotop, bitesize, seeksize, biopattern, biostacks, bioerr, mdflush, iosched, scsilatency, scsiresult, nvmelatency |
| 9.4 | BPF 单行程序（BCC / bpftrace / 单行程序示例） |
| 9.5 | 可选练习 |
| 9.6 | 小结 |

## 第 10 章 网络（411–515 页）

| 节 | 标题 |
|---|---|
| 10.1 | 背景知识（网络基础 / BPF 分析能力 / 分析策略 / 常见的跟踪错误） |
| 10.2 | 传统工具（ss / ip / nstat / netstat / sar / nicstat / ethtool / tcpdump / /proc） |
| 10.3 | BPF 工具：sockstat, sofamily, soprotocol, soconnect, soaccept, socketio, socksize, sormem, soconnlat, solstbyte, tcpconnect, tcpaccept, tcplife, tcptop, tcpsnoop, tcpretrans, tcpsynbl, tcpwin, tcpnagle, udpconnect, gethostlatency, ipecn, superping, qdisc-fq, qdisc-*, netsize, nettxlat, skbdrop, skblife, ieee80211scan, 其他工具 |
| 10.4 | BPF 单行程序（BCC / bpftrace / 单行程序示例） |
| 10.5 | 可选练习 |
| 10.6 | 小结 |

## 第 11 章 安全（516–544 页）

| 节 | 标题 |
|---|---|
| 11.1 | 背景知识（BPF 分析能力 / 无特权 BPF 用户 / 配置 BPF 安全策略 / 分析策略） |
| 11.2 | BPF 工具：execsnoop, elfsnoop, modsnoop, bashreadline, shellsnoop, ttysnoop, opensnoop, eperm, tcpconnect+tcpaccept, tcpreset, capable, setuids |
| 11.3 | BPF 单行程序（BCC / bpftrace / 单行程序示例） |
| 11.4 | 小结 |

## 第 12 章 编程语言（545–619 页）

| 节 | 标题 |
|---|---|
| 12.1 | 背景知识（编译型 / 即时编译型 / 解释型语言；BPF 分析能力 / 分析策略 / BPF 工具） |
| 12.2 | C（函数符号 / 调用栈 / 函数跟踪 / 函数偏移量跟踪 / USDT / 单行程序） |
| 12.3 | Java（跟踪 libjvm, jnistacks, 线程名字, 方法符号, 调用栈, USDT 探针, profile, offcputime, stackcount, javastat, javathreads, javacalls, javaflow, javagc, javaobjnew, Java 单行程序） |
| 12.4 | bash shell（函数计数 / 函数参数跟踪 bashfunc.bt / 函数执行时长 bashfunclat.bt / /bin/bash / bash USDT / bash 单行程序） |
| 12.5 | 其他语言（JavaScript Node.js / C++ / Golang） |
| 12.6 | 小结 |

## 第 13 章 应用程序（620–664 页）

| 节 | 标题 |
|---|---|
| 13.1 | 背景知识（应用程序基础信息 / 示例 MySQL 服务器 / BPF 分析能力 / 分析策略） |
| 13.2 | BPF 工具：execsnoop, threadsnoop, profile, threaded, offcputime, offcpuhist, syscount, ioprofile, libc 指针, mysqld_qslower, mysqld_clat, signals, killsnoop, pmlock+pmheld, naptime, 其他工具 |
| 13.3 | BPF 单行程序（BCC / bpftrace / 单行程序示例） |
| 13.4 | 小结 |

## 第 14 章 内核（665–700 页）

| 节 | 标题 |
|---|---|
| 14.1 | 背景知识（内核基础 / BPF 分析能力） |
| 14.2 | 分析策略 |
| 14.3 | 传统工具（Ftrace / perf sched / slabtop / 其他工具） |
| 14.4 | BPF 工具：loads, offcputime, wakeuptime, offwaketime, mlock+mheld, 自旋锁, kmem, kpages, memleak, slabratetop, numamove, workq, 小任务, 其他工具 |
| 14.5 | BPF 单行程序（BCC / bpftrace） |
| 14.6 | BPF 单行程序示例（按系统调用函数计数 / 对 hrtimer 计数） |
| 14.7 | 挑战 |
| 14.8 | 小结 |

## 第 15 章 容器（701–718 页）

| 节 | 标题 |
|---|---|
| 15.1 | 背景知识（BPF 分析能力 / 挑战 / 分析策略） |
| 15.2 | 传统工具（主机分析 / 容器内分析 / systemd-cgtop / kubectl top / docker stats / /sys/fs/cgroups / perf） |
| 15.3 | BPF 工具：runqlat, pidnss, blkthrot, overlayfs |
| 15.4 | BPF 单行程序 |
| 15.5 | 可选练习 |
| 15.6 | 小结 |

## 第 16 章 虚拟机管理器（719–737 页）

| 节 | 标题 |
|---|---|
| 16.1 | 背景知识（BPF 分析能力 / 建议的分析策略） |
| 16.2 | 传统工具 |
| 16.3 | 访客系统的 BPF 工具（Xen 超级调用 / xenhyper / Xen 回调 / cpustolen / HVM 退出跟踪） |
| 16.4 | 宿主机 BPF 工具（kvmexits / 未来的工作） |
| 16.5 | 小结 |

## 第 17 章 其他 BPF 性能工具（738–755 页）

| 节 | 标题 |
|---|---|
| 17.1 | Vector 和 Performance Co-Pilot（PCP）：可视化 / 热图 / 表格数据 / BCC 提供的指标 / 内部实现 / 安装 / 连接显示 / 配置 BCC PMDA / 改进 / 阅读 |
| 17.2 | Grafana 和 Performance Co-Pilot |
| 17.3 | Cloudflare eBPF Prometheus Exporter（配合 Grafana） |
| 17.4 | kubectl-trace（跟踪节点 / 跟踪 pod 和容器） |
| 17.5 | 其他工具 |
| 17.6 | 小结 |

## 第 18 章 建议、技巧和常见问题（756–769 页）

| 节 | 标题 |
|---|---|
| 18.1 | 典型事件的频率和额外开销（频率 / 执行的操作 / 自行测试） |
| 18.2 | 以 49Hz 或 99Hz 为采样频率 |
| 18.3 | 黄猪和灰鼠（性能分析的反模式） |
| 18.4 | 开发目标软件 |
| 18.5 | 学习系统调用 |
| 18.6 | 保持简单 |
| 18.7 | 事件缺失 |
| 18.8 | 调用栈缺失（如何修复损坏的调用栈） |
| 18.9 | 打印时符号缺失（如何修复：JIT 运行时 / ELF 二进制） |
| 18.10 | 跟踪时函数缺失 |
| 18.11 | 反馈回路 |
| 18.12 | 被丢掉的事件 |

## 附录（770 页起）

| 附录 | 标题 | 页码 |
|---|---|---|
| A | bpftrace 单行程序 | 770–774 |
| B | bpftrace 备忘单 | 775–777 |
| C | BCC 工具的开发 | 778–792 |
| D | C 语言 BPF | 793–811 |
| E | BPF 指令 | 812–858 |

---

## OCR 页码换算表（笔记写作用）

| 章 | 印刷页 | bpt_txt 文件 |
|---|---|---|
| 1 | 1–15 | page-040 ~ page-054 |
| 2 | 16–70 | page-055 ~ page-109 |
| 3 | 71–90 | page-110 ~ page-129 |
| 4 | 91–136 | page-130 ~ page-175 |
| 5 | 137–190 | page-176 ~ page-229 |
| 6 | 191–254 | page-230 ~ page-293 |
| 7 | 255–290 | page-294 ~ page-329 |
| 8 | 291–360 | page-330 ~ page-399 |
| 9 | 361–410 | page-400 ~ page-449 |
| 10 | 411–515 | page-450 ~ page-554 |
| 11 | 516–544 | page-555 ~ page-583 |
| 12 | 545–619 | page-584 ~ page-658 |
| 13 | 620–664 | page-659 ~ page-703 |
| 14 | 665–700 | page-704 ~ page-739 |
| 15 | 701–718 | page-740 ~ page-757 |
| 16 | 719–737 | page-758 ~ page-776 |
| 17 | 738–755 | page-777 ~ page-794 |
| 18 | 756–769 | page-795 ~ page-808 |
| 附录 A | 770–774 | page-809 ~ page-813 |
| 附录 B | 775–777 | page-814 ~ page-816 |
| 附录 C | 778–792 | page-817 ~ page-831 |
| 附录 D | 793–811 | page-832 ~ page-850 |
| 附录 E | 812–858 | page-851 ~ page-857 |
