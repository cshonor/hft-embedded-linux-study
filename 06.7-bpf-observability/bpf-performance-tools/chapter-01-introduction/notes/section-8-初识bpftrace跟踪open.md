# 1.8 初识 bpftrace：跟踪 open()

> 底本：《BPF之巅》中文版 1.8 节（PDF p50–52）

## 第一个单行程序

用静态插桩点 `syscalls:sys_enter_open` 跟踪 open(2)：

```bash
# bpftrace -e 'tracepoint:syscalls:sys_enter_open fprintf("%s %s\n", comm, str(args->filename));'
Attaching 1 probe...
slack /run/user/1000/gdm/Xauthority
slack /run/user/1000/gdm/Xauthority
...
```

- 程序写在**单引号**内，敲 Enter **立即编译并运行**；Ctrl+C 结束时探针禁用、BPF 程序移除——**按需插桩，观测窗口可短至几秒**
- 全系统层面：任何调用 open(2) 的应用都被覆盖
- 这是"**输出单个事件**"类工具：一行 = 一次系统调用
- 作者实时观察到了自己笔记本上 Slack 打开的文件——BPF 不只用于服务器

## 发现遗漏：open 有变体

输出比预想慢——因为只跟踪了 open 的**一个变体**。列出所有相关跟踪点：

```bash
# bpftrace -l 'tracepoint:syscalls:sys_enter_open*'
tracepoint:syscalls:sys_enter_open
tracepoint:syscalls:sys_enter_openat
```

用计数单行程序验证谁才是主力：

```bash
# bpftrace -e 'tracepoint:syscalls:sys_enter_open* { @[probe] = count(); }'
Attaching 3 probes...
@[tracepoint:syscalls:sys_enter_open]:    5
@[tracepoint:syscalls:sys_enter_openat]: 308
```

- `@[probe] = count()` 是**映射表（map）计数**——摘要由 BPF 程序**在内核中高效计算**，不是把事件搬回用户态再数
- 结论：openat(2)（308 次）才是现代程序主力，open(2)（5 次）已边缘化

## 从单行到脚本：opensnoop.bt

同时跟踪 open + openat 后单行变长，更好的方式是脚本。bpftrace 自带 **opensnoop.bt**，同时跟踪每个系统调用的**开始和结束**位置，分列输出：

```text
# opensnoop.bt
COMM     PID   FD  ERR PATH
2440            0   /proc/cpuinfo
25706    ls     0   /lib/x86_64-linux-gnu/libc.so.6
25706    ls     0   /usr/lib/locale/locale-archive
1744     snmpd  0   /proc/net/dev
1744     snmpd  2   /sys/class/net/lo/device/vendor
```

列含义：PID / 进程命令名 COMM / 文件描述符 FD / 错误码 ERR / 路径 PATH。用途：排查出错的软件（打开了错误位置）、摸清配置和日志文件位置、识别**打开频次过高/反复检查错误路径**的性能问题。

> bpftrace 自带 20+ 工具，BCC 自带 70+；工具源码同时是"如何跟踪某类事件"的教学材料。

---

### 常见陷阱

1. **跟踪 open(2) 却看不到流量** —— 忘了 openat 变体。教训泛化：跟踪系统调用家族时**先 `-l` 列出全部变体**再挂探针（read/pread64、stat/statx/newfstatat、send/sendto/sendmsg 同理）
2. 单引号内的程序里再用单引号会截断——bash 层面注意引号嵌套

### HFT 关联

- 行情/交易进程启动慢或读错配置文件，`opensnoop.bt` 三秒给出**实际打开路径序列**——比猜配置加载顺序快得多
- `@[probe] = count()` 内核态计数模式是低开销全系统事件计数的标准范式，行情风暴时统计各事件 QPS 不加重用户态负担
- 交叉引用：bpftrace 语言细节见 [Ch 5](../chapter-05-bpftrace/)，单行程序全集见附录 A

<details>
<summary>📝 自测题（点击展开）</summary>

1. **为什么跟踪 `sys_enter_open` 看到的文件打开远少于预期？如何一次确认全部变体？**

   <details><summary>参考答案</summary>

   现代程序主要用 openat(2) 而非 open(2)。用 `bpftrace -l 'tracepoint:syscalls:sys_enter_open*'` 列出家族全部跟踪点，再用 `@[probe] = count()` 分别计数确认主力变体。

   </details>

2. **opensnoop.bt 为什么要同时跟踪系统调用的开始和结束位置？**

   <details><summary>参考答案</summary>

   开始位置提供参数（路径、flags），结束位置提供返回值（FD 或 errno）——配对后才能输出 FD/ERR 列。这也是 uprobe/kprobe 计时类工具的通用 entry/exit 配对模式。

   </details>

</details>
