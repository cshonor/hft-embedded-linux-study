# 5.4 bpftrace 工具

> 底本：《BPF之巅》第 5 章 bpftrace（印刷 p137–190），5.4 节（印刷 p143–144）

## 内容详解

### 图 5-2：按系统组件组织的工具全景

工具覆盖整个栈：应用程序 / 编程语言运行时 / 系统库 / 系统调用接口 / 虚拟文件系统 / 文件系统 / 卷管理器 / TCP·UDP / IP / 调度器 / 虚拟内存 / 块设备 / 网络设备 / 设备驱动。图中黑色 = bpftrace 仓库已有工具，灰色 = 本书新增（变体工具未列，如第 10 章的 qdisc 变体）。

这张图与本仓库 ch01 的"BPF 跟踪能见度"全景图是**同一张地图的两种画法**：ch01 画的是"哪里能插桩"（技术视角），这里画的是"哪里已有现成工具"（货架视角）。排障时的用法：先在 ch01 图上确认目标层有观测手段，再回这张图查货架——货架上没有的才需要自己写单行。

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

**按症状反查这张表**（症状 → 首查工具）：

| 症状 | 首查 | 逻辑 |
|------|------|------|
| 延迟毛刺、尾延迟抖动 | runqlat.bt、offcputime.bt（ch6） | 先分清"在 CPU 上慢"还是"在等" |
| 进程莫名消失/被杀 | execsnoop.bt、oomkill.bt（ch6/7） | 谁启动了它 / 谁杀了它 |
| 磁盘 I/O 慢 | biolatency.bt、bitesize.bt（ch9） | 延迟分布 + 是否小 I/O 碎片化 |
| 连接异常（重传/被拒） | tcpretrans.bt、tcpdrop.bt（ch10） | 网络丢包/栈内主动 drop |
| 文件打开失败 | opensnoop.bt（ch8） | ERR 列直接给 errno |
| 睡眠过头（定时器漂移） | naptime.bt（ch13） | nanosleep 实际睡了多久 |

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

读输出的三个约定（后续章节通用，先建立习惯）：

1. **Attaching N probes** 是健康信号——N 符合预期说明通配符展开成功；N=0 或报错先看探针名；
2. **ERR 列**是 errno（0=成功），opensnoop/filelife 族工具的排障主入口；
3. **Hit Ctrl-C to end** 提醒你：多数工具在内核 map 里聚合，Ctrl-C 才打印摘要——挂在后台忘了就是白跑（自动化场景配 interval+exit，见 5.5）。

## HFT 关联

- 图 5-2 + 表 5-1 是"症状→工具"的索引页：先按资源域定位章节（如延迟抖动 → ch6 CPU 工具），再进对应章读详解；
- bpftrace 工具短小（.bt 通常几十行）可直接读源码学习改写——这正是后续章节全部附源码的原因；
- .bt 工具 = **脚本即文档**：交易机的 runbook 里不需要额外维护一份工具说明，注明工具名 + man 页 + 关键参数即可；改工具 = 改脚本，diff 可审——比二进制工具友好得多。

## 陷阱

- ⚠️ 表 5-1 的工具是 bpftrace 版；同名的 BCC 版（如 biolatency）行为/参数不同，runbook 中注明 `.bt`。
- ⚠️ "Attaching 5 probes" 里探针数取决于内核版本与编译符号——同一脚本在不同内核上探针数不同属正常，不是脚本坏了。

<details>
<summary>自测题</summary>

1. bpftrace 工具设计上的两个"文档标配"是什么？
   <details><summary>答案</summary>man 8 手册页 + *_example.txt 示例文件。</details>

2. 为什么 bpftrace 工具强调"简单短小"？
   <details><summary>答案</summary>语言定位即临时观测；复杂工具应交给 BCC 实现。</details>

3. 交易机盘后发现"某策略进程消失"，按症状反查表先跑哪两个工具？
   <details><summary>答案</summary>execsnoop.bt（谁启动的、带了什么参数）+ oomkill.bt（是否被 OOM 杀掉；两者分别对应 ch6/ch7）。</details>
</details>
