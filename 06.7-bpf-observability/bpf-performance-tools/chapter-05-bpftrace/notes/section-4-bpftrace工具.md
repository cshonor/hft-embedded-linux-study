# 5.4 bpftrace 工具

> 底本：《BPF之巅》第 5 章 bpftrace（印刷 p137–190），5.4 节（印刷 p143–144）

## 内容详解

### 图 5-2：按系统组件组织的工具全景

工具覆盖整个栈：应用程序 / 编程语言运行时 / 系统库 / 系统调用接口 / 虚拟文件系统 / 文件系统 / 卷管理器 / TCP·UDP / IP / 调度器 / 虚拟内存 / 块设备 / 网络设备 / 设备驱动。图中黑色 = bpftrace 仓库已有工具，灰色 = 本书新增（变体工具未列，如第 10 章的 qdisc 变体）。

### 5.4.1 重点工具（表 5-1，按主题 × 章节）

| 主题 | 特色工具 | 章节 |
|------|----------|------|
| CPU | execsnoop.bt、runqlat.bt、runqlen.bt、cpuwalk.bt、offcputime.bt | 6 |
| 内存 | oomkill.bt、failts.bt、vmscan.bt、swapin.bt | 7 |
| 文件系统 | vfsstat.bt、filelife.bt、xfsdist.bt | 8 |
| 存储 I/O | biosnoop.bt、biolatency.bt、bitesize.bt、biostacks.bt、scsilatency.bt、nvmelatency.bt | 9 |
| 网络 | tcpaccept.bt、tcpconnect.bt、tcpdrop.bt、tcpretrans.bt、gethostlatency.bt | 10 |
| 安全 | ttysnoop.bt、elfsnoop.bt、setuids.bt | 11 |
| 编程语言 | jnistacks.bt、javacalls.bt | 12 |
| 应用程序 | threadsnoop.bt、pmheld.bt、naptime.bt、mysqld_qslower.bt | 13 |
| 内核 | mlock.bt、mheld.bt、kmem.bt、kpages.bt、workq.bt | 14 |
| 容器 | pidnss.bt、blkthrot.bt | 15 |
| 虚拟机管理器 | xenhyper.bt、cpustolen.bt、kvmexits.bt | 16 |
| 调试/多用途 | execsnoop.bt、threadsnoop.bt、opensnoop.bt、killsnoop.bt、signals.bt | 6、8、13 |

（BCC 工具不在表 5-1 中；本书后续章节按观测目标把 BCC 与 bpftrace 工具一起讲，可当参考手册查。）

### 5.4.2 工具特征

- 解决真实世界的观测问题；
- 设计为**生产环境中由 root 使用**；
- 每工具一份 man 手册（man/man8）+ 示例文件（tools/*_example.txt）；
- 工具源码以注释开头（说明用途）；
- **尽量简单短小**——更复杂的工具交给 BCC。

### 5.4.3 工具的运行

```bash
$ sudo ./opensnoop.bt
Attaching 5 probes..
Tracing open syscalls... Hit Ctrl-C to end.
PID    COMM  FD ERR PATH
25612  bpftrace  23  0  /dev/null
...
```

- 不加 sudo 直接跑会报错：`ERROR: bpftrace currently only supports running as the root user.`；
- 工具可拷入 `/usr/local/sbin` 等目录与其他系统工具并列。

## HFT 关联

- 图 5-2 + 表 5-1 是"症状→工具"的索引页：先按资源域定位章节（如延迟抖动 → ch6 CPU 工具），再进对应章读详解；
- bpftrace 工具短小（.bt 通常几十行）可直接读源码学习改写——这正是后续章节全部附源码的原因。

## 陷阱

- ⚠️ 表 5-1 的工具是 bpftrace 版；同名的 BCC 版（如 biolatency）行为/参数不同，runbook 中注明 `.bt`。

<details>
<summary>自测题</summary>

1. bpftrace 工具设计上的两个"文档标配"是什么？
   <details><summary>答案</summary>man 8 手册页 + *_example.txt 示例文件。</details>

2. 为什么 bpftrace 工具强调"简单短小"？
   <details><summary>答案</summary>语言定位即临时观测；复杂工具应交给 BCC 实现。</details>
</details>
