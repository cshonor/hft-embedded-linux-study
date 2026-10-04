# 8.3 BPF 工具：系统调用层跟踪（8.3.1–8.3.6）

> 底本：《BPF之巅》第 8 章 文件系统，8.3 节前半（印刷 p303–313）

系统调用层工具一览：

| 工具 | 来源 | 跟踪对象 | 一句话 |
|---|---|---|---|
| opensnoop | BCC/BT | open(2)/openat(2) | 谁打开了什么文件 |
| statsnoop | BCC/BT | stat 全家族 | 谁在反复 stat 哪些文件 |
| syncsnoop | BCC/BT | sync/fsync/fdatasync 家族 | 谁在触发刷盘 |
| mmapfiles | BT | do_mmap 内核函数 | 哪些文件被内存映射 |
| scread | BT | read(2) + FD→文件名 | 按文件名统计读取次数 |
| fmapfault | BT | filemap_fault | 内存映射文件的缺页统计 |

系统调用事件频率低（相对 VFS/页缓存层），这一组工具开销均可忽略。

## opensnoop

```
# opensnoop -T      # 生产系统实例
TIME(s)   PID    COMM  FD ERR  PATH
0.000000  5248   java  0   0    /proc/loadavg
0.000036  5248   java  0   0    /sys/fs/cgroup/.../cpu.cfs_quota_us
...
```

**书例（生产事故）**：java 进程每秒打开同一组 4 个监控文件（/proc/loadavg + 3 个 cgroup 文件）超过 100 次。下一步用针对该 PID 的 kprobe 栈跟踪（第 18 章方法抓 java 用户态栈），最终定位是**新引入的负载均衡软件**在空转轮询。

- 跟踪点：`tracepoint:syscalls:sys_enter_open{,at}`，入口存文件名，出口配 FD/ERR 打印。
- 选项：`-x` 只看失败打开；`-P PID`；`-n NAME` 按进程名过滤。
- 历史注脚：第一版 opensnoop.d 写于 2004-03-09（DTrace 时代），"snoop" 源自 Solaris 网络监听器。

## statsnoop

跟踪 stat(2) 变体：statfs/statx/newstat/newlstat。

**书例（生产事故）**：Netflix 微服务磁盘 100% I/O——一个磁盘监控程序不停对**大型文件系统**逐个 stat，元数据装不进缓存，每个 stat 都变成一次磁盘 I/O。同类还有 systemd-resolved 每秒对 resolv.conf 三件套循环 stat（无害但无谓）。

- 选项：`-x`（只看失败，找"文件不存在"问题）、`-t`、`-P PID`。
- 判断标准：stat 频率高 + 命中缓存 = 无害；stat 落盘 = I/O 放大器。

## syncsnoop

跟踪 sync(2)/syncfs(2)/fsync(2)/fdatasync(2)/sync_file_range(2)/msync(2)。

```
08:48:31  14172  TaskSchedulerFo  sys_enter_fdatasync   ×5 连发
```

同步调用会触发磁盘写队列，是**延迟毛刺**的经典来源——syncsnoop 只记秒级时间戳，目的就是拿输出和监控软件记录的延迟尖峰对时间轴。确认后可再用自定义 bpftrace 打印参数、返回值与对应磁盘 I/O。

## mmapfiles

```
@[lib,...,libc-2.23.so]:2879      # 编译负载
@[,]:8384                          # 无名键 = 匿名映射（程序私有数据）
```

kprobe `do_mmap`，从 `struct file *` 参数经 dentry 取文件名，并带上两级父目录定位。键可扩展为 `@[comm,...]` 或加 ustack。对比第 7 章 mmapsnoop（逐事件看 mmap 系统调用），本工具是按文件聚合统计。

## scread —— FD 转文件名的教科书示例

```bash
tracepoint:syscalls:sys_enter_read
  $task = (struct task_struct *)curtask;
  $file = (struct file *)*( $task->files->fdt->fd + args->fd );
  @filename[str($file->f_path.dentry->d_name.name)] = count();
```

**FD→文件名的两种方法（8.3.5 插栏，全书通用知识点）**：

| 方法 | 原理 | 优缺点 |
|---|---|---|
| ① FD 表指针穿行 | task->files->fdt->fd[fd] 取 file 结构体 | 无额外探针；但依赖内核内部布局，**跨版本不稳** |
| ② open 时建表 | 跟踪 open(2) 维护 (PID,FD)→文件名哈希 | 稳定；多一个探针的开销 |

跟踪 VFS 层函数（fileslower/filetop 等）则天然有 `struct file*` 参数，比系统调用层拿文件名更省事，且不受 read/readv/preadv/pread64 等变体增殖之苦。

## fmapfault

```
@[as,libopcodes-2.26.1-system.so]:68455   # 编译时 as 汇编器狂缺页
```

kprobe `filemap_fault`，经 `vm_fault->vma->vm_file` 穿行取文件名，按 (comm, 文件) 统计。**它补上了 read(2) 之外的另一半 I/O**：mmap 文件不经过 read 系统调用，读写次数可能远高于缺页次数（缺页只是首次触页）。缺页高频系统上本工具有可感知开销。

## HFT 关联

- opensnoop/statsnoop 是"低配监控审计"利器：确认没有进程在热路径反复 open/stat（每次 open 都要做路径查找+inode 分配，是慢操作）。
- fmapfault 对检查行情快照 mmap 加载、.so 首次换页很有用。
- syncsnoop 用来抓"谁在整点 fsync 抖一下"。

## 常见陷阱

- statsnoop 高频 stat 是否有害取决于是否落盘，先看 cachestat/iostat 再下结论。
- fmapfault 键为空 = 匿名内存，不是 bug。

<details>
<summary>自测</summary>

1. opensnoop 抓到每秒 100 次 open，如何进一步定位是哪段代码在打开？
2. FD 转文件名的两种方案各自的稳定性/开销权衡？
3. 为什么说 fmapfault 补充了 read(2) 视角的盲区？
</details>
