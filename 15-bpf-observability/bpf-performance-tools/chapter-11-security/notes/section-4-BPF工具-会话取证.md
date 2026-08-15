# 4. BPF 工具：会话取证（11.2.5–11.2.6）

> 底本：《BPF之巅》第 11 章 安全，11.2 节（印刷 p528–531）

## 4.1 shellsnoop —— 镜像 shell 会话输出（11.2.5）

```
# shellsnoop 7866
bgregg:~> date
Fri May 31 18:11:02 PDT 2019
bgregg:~> echo Hello BPF
Hello BPF
```

- 镜像指定 PID shell 会话的**命令与输出**（含子进程输出，如 date(1) 的结果）——跟踪该进程及其子进程对 STDOUT/STDERR 的 write。
- **-r 选项：生成可重放脚本**——输出 `echo ...; sleep <间隔>` 序列，存盘后 bash 执行即按原始时间重放会话（"有点诡异"）。
- BCC 选项：-S 仅 shell 输出（不含子命令）；-r 重放脚本。
- bpftrace 版核心逻辑：

```bash
t:sched:sched_process_fork  { @descendent[args->child_pid] = 1; }
t:syscalls:sys_enter_write   /@descendent[pid]/ { printf("%s", str(args->buf, args->count)); }
```

- 历史：灵感来自 ttywatcher（2004 年 cuckoo.d），曾在 Phrack 上被用于安全分析。
- 局限：输出截断为 BPFTRACE_STRLEN（64 字节）。

## 4.2 ttysnoop —— 镜像 TTY/PTS 设备（11.2.6）

```
# ttysnoop 16            # 观察 /dev/pts/16
$ gcc -o a.out crack.c
$ ./a.out
Segmentation fault
```

- 跟踪 `tty_write()` 内核函数并打印写入内容——实时观察可疑登录会话。
- 作者轶事：当系统管理员时用 ttywatcher 实时围观入侵者下载提权漏洞并编译失败（最烦人的是对方用 pico 而不是 vi）。灵感源自 Stoll 的《Cuckoo's Egg》。
- 设备参数：全路径 `/dev/pts/2`、数字 `2`、或 `/dev/tty0`；`/dev/console` 显示系统控制台。选项 -c 不清屏。
- bpftrace 版：kprobe tty_write，`$file->f_path.dentry->d_name.name` 与 `$1+3`（跳过 "pts" 前缀）比对设备名。
- **必须指定设备**：跟踪所有设备会把输出混流并与工具自身输出形成反馈回路。

## HFT 关联

- 共置/托管环境下合规审计：ttysnoop 记录交易所机房驻场操作会话，满足操作留痕要求。
- shellsnoop -r 的"会话重放"可用于事后复盘误操作事故的时间线。

<details>
<summary>自测题</summary>

1. shellsnoop 如何捕获子进程输出？
2. ttysnoop 为什么必须指定设备？跟踪什么内核函数？
3. -r 选项生成的脚本有什么用？
</details>
