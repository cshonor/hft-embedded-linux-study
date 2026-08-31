# 8.4 BPF 单行程序

> 底本：《BPF之巅》第 8 章 文件系统，8.4 节（印刷 p353–358）

## BCC 版（funccount / argdist / trace / stackcount）

```bash
# 按进程名跟踪 creat(2)
trace 't:syscalls:sys_enter_creat' '%s', args->pathname

# 按文件名统计 newstat(2)
argdist -e 't:syscalls:sys_enter_newstat():char*:args->filename'

# 按系统调用变体统计 read / write（识别哪个变体在被用）
funccount 't:syscalls:sys_enter_*read*'
funccount 't:syscalls:sys_enter_*write*'

# 按错误码统计 read 错误
argdist -c 't:syscalls:sys_exit_read():int:args->ret:args->ret<0'

# 统计 VFS / ext4 / XFS 跟踪点
funccount 'vfs*'
funccount 't:ext4:*'
funccount 't:xfs:*'

# 按进程名(+用户态栈)统计 ext4 读
stackcount ext4_file_read_iter
stackcount -u ext4_file_read_iter

# 按调用栈统计预读取落盘路径
stackcount -P ext4_readpages
```

## bpftrace 版

```bash
# 按进程名统计 open
bpftrace -e 't:syscalls:sys_enter_open { printf("%s %s\n", comm, str(args->filename)); }'

# 按文件名统计 newstat
bpftrace -e 't:syscalls:sys_enter_newstat / @{str(args->filename)} = count(); /'

# 按变体统计 read/write
bpftrace -e 'tracepoint:syscalls:sys_enter_*write* { @[probe] = count(); }'

# read 返回字节数直方图（含错误）
bpftrace -e 'tracepoint:syscalls:sys_exit_read { @ = hist(args->ret); }'

# ext4 读按进程/用户栈
bpftrace -e 'kprobe:ext4_file_read_iter { @[ustack, comm] = count(); }'

# ZFS spa_sync 时间点
bpftrace -e 'kprobe:spa_sync { time("%H:%M:%S ZFS spa_sync\n"); }'

# 按调用栈统计 readpages
bpftrace -e 'kprobe:read_pages { @[kstack] = count(); }'
```

## 示例解读（8.4.3，三个必记案例）

**① read 变体普查**（36-CPU 生产）：

```
sys_enter_read          9863782   ← 10 秒内 read(2) 一千万次，主力
sys_enter_readlinkat    34
```

用途：搞清楚该审查哪个变体。

**② read 返回字节数直方图**：

```
[1]        15609        ← 15609 次只读了 1 字节！优化目标
[0]        2899         ← EOF，正常
负值        279          ← 错误，单独分析
[4K,8K]    23926        ← 主体
```

1 字节读抓栈：`bpftrace -e 't:syscalls:sys_exit_read /args->ret == 1/ { @[ustack] = count(); }'`

**③ XFS 跟踪点普查**：XFS 跟踪点太多（约 500 个），funccount 输出截断——xfs_ilock 581785、xfs_writepage 476196 等，用于深挖 XFS 内部行为。

**④ stackcount -P ext4_readpages 的两个栈**：

```
栈1: ext4_readpages ← read_pages ← do_page_cache_readahead ← filemap_fault
     ← handle_mm_fault ← do_page_fault ← ... ← execve   ← mmap 缺页触发预读
栈2: ext4_readpages ← ondemand_readahead ← generic_file_read_iter ← vfs_read
     ← ... ← execve                                      ← read(2) 触发预读
```

同一底层函数（ext4_readpages）有**两条触发路径**：地址空间缺页与显式读——内核栈把来龙去脉讲得明明白白（Linux 4.18 栈，跨版本会变）。

## HFT 关联

- 1 字节读检测直击行情/协议解析代码的 syscalls 次数浪费，抓 ustack 即得代码位置。
- `*read*`/`*write*` 变体普查用于确认没有意外的 preadv/readv 混用路径。

<details>
<summary>自测</summary>

1. read 返回 0、1、负值各代表什么？哪个最值得优化？
   <details><summary>答案</summary>0=EOF（正常读完）；1=只读了 1 字节——**最值得优化**：一次系统调用（百 ns~µs 级固定成本）只搬 1 字节数据，是调用粒度的浪费（书例 15609 次 1 字节读，抓 ustack 即定位到代码行）；负值=-errno，错误另案分析（看是 EAGAIN 这类可重试还是真错）。</details>

2. ext4_readpages 的两条触发路径分别是什么系统调用/异常引起的？
   <details><summary>答案</summary>栈1：mmap 后的**缺页异常**（handle_mm_fault→filemap_fault→预读把周围页一起拉进缓存）——read 系统调用都没发生；栈2：**显式 read(2)**（vfs_read→generic_file_read_iter→ondemand_readahead）。同一预读函数服务两种触发，这也是"文件 I/O 不只发生在 read 调用里"的实证——只审计 syscall 看不到 mmap 缺页路径的 I/O。</details>
</details>
